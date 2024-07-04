
use strict;
use warnings;

use Test::More;
use Prima::sys::Test qw(noX11);

sub is_pict
{
	my ( $i, $name, $pict ) = @_;
	my $ok = 1;
	my $m = $i-> matrix;
	$m->save;
	$m->reset;
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
	if ($ok) {
		$m->restore;
		return 1;
	}
	warn "# Actual vs expected:\n";
	for ( my $y = 0; $y < $i->height; $y++) {
		my $actual   = join '', map { ($i->pixel($_,$i->height-$y-1) > 0) ? '*' : ' ' } 0..$i->width-1;
		my $expected = substr($pict, $y * $i->width, $i->width);
		warn "$actual  | $expected\n";
	}
	$m->restore;
	return 0;
}

# check optimizers 
for my $bpp ( 1, 4, 8, 24 ) {
	my $i = Prima::Image->create(
		width     => 5,
		height    => 5,
		type      => $bpp,
		color     => cl::White,
		backColor => cl::Black,
	);

	$i->clear;
	$i->line(1,1,3,1);
	is_pict($i, "$bpp: unclipped hline",
		"     ".
		"     ".
		"     ".
		" *** ".
		"     "
	);
	
	$i->clear;
	$i->line(-1,1,3,1);
	is_pict($i, "$bpp: left clipped hline",
		"     ".
		"     ".
		"     ".
		"**** ".
		"     "
	);

	$i->clear;
	$i->line(1,1,9,1);
	is_pict($i, "$bpp: left clipped hline",
		"     ".
		"     ".
		"     ".
		" ****".
		"     "
	);

	$i->clear;
	$i->line(-1,1,9,1);
	is_pict($i, "$bpp: clipped hline",
		"     ".
		"     ".
		"     ".
		"*****".
		"     "
	);
	
	$i->clear;
	$i->rop(rop::XorPut);
	$i->rectangle( 1,1,3,3);
	is_pict($i, "$bpp: rectangle",
		"     ".
		" *** ".
		" * * ".
		" *** ".
		"     "
	);
}

# those are unoptimized
my $i = Prima::Image->create(
	width     => 5,
	height    => 5,
	type      => im::bpp1,
	color     => cl::White,
	backColor => cl::Black,
);
$i->clear;
$i->line(1,1,3,3);
is_pict($i, "line",
	"     ".
	"   * ".
	"  *  ".
	" *   ".
	"     "
);


$i->clear;
$i->linePattern(lp::DotDot);
$i->rop2(rop::NoOper);
$i->line(1,1,3,3);
$i->linePattern(lp::Solid);
is_pict($i, "line dotted transparent",
	"     ".
	"   * ".
	"     ".
	" *   ".
	"     "
);

$i->clear;
$i->linePattern(lp::DotDot);
$i->rop2(rop::CopyPut);
$i->line(1,1,3,3);
$i->linePattern(lp::Solid);
is_pict($i, "line dotted opaque white",
	"     ".
	"   * ".
	"     ".
	" *   ".
	"     "
);

$i->clear;
$i->backColor(cl::White);
$i->linePattern(lp::DotDot);
$i->rop2(rop::CopyPut);
$i->line(1,1,3,3);
$i->backColor(cl::Black);
$i->linePattern(lp::Solid);
is_pict($i, "line dotted opaque black",
	"     ".
	"   * ".
	"  *  ".
	" *   ".
	"     "
);

$i->clear;
$i->region( Prima::Region->new( box => [2,2,1,1]));
$i->line(1,1,3,3);
is_pict($i, "line with simple region",
	"     ".
	"     ".
	"  *  ".
	"     ".
	"     "
);
$i->region( undef );

$i->clear;
$i->region( Prima::Region->new( box => [1,1,1,1, 3,3,1,1]));
$i->line(1,1,3,3);
is_pict($i, "line with complex region",
	"     ".
	"   * ".
	"     ".
	" *   ".
	"     "
);
$i->region( undef );

$i->clear;
$i->region( Prima::Region->new( box => [10,10,10,10]));
$i->line(1,1,3,3);
is_pict($i, "line outside region",
	"     ".
	"     ".
	"     ".
	"     ".
	"     "
);
$i->region( undef );

$i->clear;
$i->region( Prima::Region->new( box => [1,1,1,1, 3,3,1,1]));
$i->translate(-1,-1);
$i->line(1,1,3,3);
is_pict($i, "line with complex region and transform",
	"     ".
	"     ".
	"     ".
	" *   ".
	"     "
);
$i->translate(0,0);
$i->region( undef );

$i->linePattern(lp::Solid);
$i->clear;
$i->ellipse(2,2,5,5);
is_pict($i, "ellipse",
	"  *  ".
	" * * ".
	"*   *".
	" * * ".
	"  *  "
);

$i->clear;
$i->arc(2,2,5,5,0,90);
is_pict($i, "arc",
	"  *  ".
	"   * ".
	"    *".
	"     ".
	"     "
);

$i->clear;
$i->chord(2,2,5,5,180,0);
is_pict($i, "chord",
	"  *  ".
	" * * ".
	"*****".
	"     ".
	"     "
);

$i->clear;
$i->sector(2,2,5,5,0,270);
is_pict($i, "sector",
	"  *  ".
	" * * ".
	"* ***".
	" **  ".
	"  *  "
);

$i->clear;
$i->lines([1,1,3,1, 1,3,3,3, 1,4,4,4]);
is_pict($i, "lines",
	" ****".
	" *** ".
	"     ".
	" *** ".
	"     "
);

$i->clear;
$i->polyline([1,1,4,1,1,4,4,4]);
is_pict($i, "polyline",
	" ****".
	"  *  ".
	"   * ".
	" ****".
	"     "
);

$i->clear;
$i->fillMode(fm::Overlay|fm::Winding);
$i->fill_ellipse(2,2,5,5);
is_pict($i, "fill_ellipse",
	"  *  ".
	" *** ".
	"*****".
	" *** ".
	"  *  "
);

$i->clear;
$i->fill_sector(2,2,5,5,0,90);
is_pict($i, "fill_sector",
	"  *  ".
	"  ** ".
	"  ***".
	"     ".
	"     "
);

$i->clear;
$i->fill_chord(2,2,5,5,0,90);
is_pict($i, "fill_chord",
	"  *  ".
	"   * ".
	"    *".
	"     ".
	"     "
);

# now with matrix
$i->matrix([-1,0,0,1,5,0]);

$i->clear;
$i->fill_chord(2,2,5,5,0,90);
is_pict($i, "fill_chord with matrix",
	"   * ".
	"  *  ".
	" *   ".
	"     ".
	"     "
);

$i->clear;
$i->polyline([1,1,4,1,1,4,4,4]);
is_pict($i, "polyline with matrix",
	" ****".
	"   * ".
	"  *  ".
	" ****".
	"     "
);

$i->clear;
$i->rectangle(1,1,4,4);
is_pict($i, "rectangle with matrix",
	" ****".
	" *  *".
	" *  *".
	" ****".
	"     "
);

$i->clear;
$i->bar(1,1,4,4);
is_pict($i, "bar with matrix",
	" ****".
	" ****".
	" ****".
	" ****".
	"     "
);

# test AA fills
sub is_pict8
{
	my ( $i, $name, $pict ) = @_;
	my $ok = 1;
	ALL: for ( my $y = 0; $y < $i->height; $y++) {
		for ( my $x = 0; $x < $i->width; $x++) {
			my $actual   = $i->pixel($x,$y);
			$actual = ($actual < 128 ? 64 : 192) if $actual > 0 && $actual < 255;
			my $expected = substr($pict, ($i->height-$y-1) * $i->width + $x, 1);
			if ( $expected eq ' ') {
				$expected = 0;
			} elsif ( $expected eq '*') {
				$expected = 255;
			} elsif ( $expected eq '.') {
				$expected = 64;
			} else {
				$expected = 192;
			}
			next if $actual == $expected;
			$ok = 0;
			last ALL;
		}
	}
	ok( $ok, $name );
	return 1 if $ok;
	warn "# Actual vs expected:\n";
	for ( my $y = 0; $y < $i->height; $y++) {
		my $actual   = join '', map {
			my $px = $i->pixel($_,$i->height-$y-1);
			if ( $px == 0 ) {
				$px = ' ';
			} elsif ( $px == 255 ) {
				$px = '*';
			} elsif ( $px < 128 ) {
				$px = '.';
			} else {
				$px = '+';
			}
			$px;
		} 0..$i->width-1;
		my $expected = substr($pict, $y * $i->width, $i->width);
		warn "$actual  | $expected\n";
	}
	return 0;
}
$i = Prima::Image->create(
	width     => 5,
	height    => 5,
	antialias => 1,
	type      => im::Byte,
	color     => cl::White,
	backColor => cl::Black,
);
$i->clear;
$i->fillpoly([1,1,3.9,1,2.5,3]);
is_pict8($i, "aa fill",
	"     ".
	"     ".
	" .+. ".
	" +*+ ".
	"     "
);

$i->clear;
$i->fillpoly([-10, -10, 15, -10, 2.5, 2]);
is_pict8($i, "aa fill oob",
	"     ".
	"     ".
	"     ".
	" .+. ".
	".+*+."
);

done_testing;
