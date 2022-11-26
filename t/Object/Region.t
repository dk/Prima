use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

my $dbm = Prima::DeviceBitmap->new( size => [5, 5], monochrome => 1);
my $r;

sub try($$)
{
	my ( $name, $expected ) = @_;
	$dbm->region(undef);
	$dbm->clear;
	$dbm->region($r);
	$dbm->bar(0,0,$dbm->size);

	my $i = $dbm->image;
	$i->type(im::Byte);
	$i->mirror(1);
	my $data = $i->data;
	$data =~ s/(.{5}).{3}/$1/gs;
	$data =~ s/[^\x00]/ /g;
	$data =~ s/\x00/*/g;
	is( $data, $expected, $name);
}

# empty
try('empty undef',"*" x 25);
$r = Prima::Region->new;
ok($r, 'empty rgn');
ok($r->is_empty, 'is empty');
try('empty def'," " x 25);
is_deeply([$r->box], [0,0,0,0], 'empty/box');

# rect
$r = Prima::Region->new( box => [0, 0, 1, 1]);
ok(!$r->is_empty, 'not empty');
try('box 1x1',
	"     ".
	"     ".
	"     ".
	"     ".
	"*    "
);
is_deeply([$r->box], [0,0,1,1], 'box 1x1/box');
$r->offset(2,2);
try('box 1x1 + 2.2',
	"     ".
	"     ".
	"  *  ".
	"     ".
	"     "
);
is_deeply([$r->box], [2,2,1,1], 'box 1x1/box + 2.2');
$r->offset(-4,-4);
is_deeply([$r->box], [-2,-2,1,1], 'box 1x1/box - 2.2');

$r = Prima::Region->new( box => [1, 1, 3, 3]);
try('box 3x3',
	"     ".
	" *** ".
	" *** ".
	" *** ".
	"     "
);
is_deeply([$r->box], [1,1,3,3], 'box 3x3/box');

my $r2 = Prima::Region->new( rect => [1, 1, 4, 4]);
ok( $r->equals($r2), 'equals');

my @star = (0, 0, 2, 5, 5, 0, 0, 3, 5, 3);
$r = Prima::Region->new(polygon => \@star, fillMode => fm::Alternate);
my @box = $r->box;
ok($box[0] < $box[2] && $box[1] < $box[3], 'star 1');

$r2 = Prima::Region->new(polygon => \@star, fillMode => fm::Winding);
@box = $r->box;
ok($box[0] < $box[2] && $box[1] < $box[3], 'star 2');
ok( !$r-> equals($r2), 'poly winding');

my $image = Prima::Image->new(
	size => [4,4],
	type => im::Byte,
	data =>
		"\xff\xff\xff\x00". # y=0
		"\xff\xff\xff\x00".
		"\xff\xff\xff\x00".
		"\x00\x00\x00\x00"  # y=3
);
$r = Prima::Region->new(image => $image);
is_deeply([$r->box], [0,0,3,3], 'image');

ok( $r-> point_inside(0,0), "point inside 0,0");
ok( $r-> point_inside(2,2), "point inside 2,2");
ok( !$r-> point_inside(2,3), "point inside 2,3");
ok( !$r-> point_inside(3,2), "point inside 3,2");

is( $r-> rect_inside(0,0,0,0), rgn::Inside, "rect inside 0,0,0,0");
is( $r-> rect_inside(1,1,2,2), rgn::Inside, "rect inside 1,1,2,2");
is( $r-> rect_inside(2,2,3,3), rgn::Partially, "rect partially inside 2,2,3,3");
is( $r-> rect_inside(3,3,3,3), rgn::Outside, "rect outside 3,3,3,3");

$dbm->region($image);
@box = $dbm->clipRect;
is_deeply(\@box, [0,0,2,2], 'region clip rect');

my $dbm2 = Prima::DeviceBitmap->new( size => [5,5], type => dbt::Pixmap );
$dbm2->region($image);
@box = $dbm->clipRect;
is_deeply(\@box, [0,0,2,2], 'region clip rect on pixmap');

if ( $::application->get_system_value(sv::LayeredWidgets)) {
	$dbm2 = Prima::DeviceBitmap->new( size => [5,5], type => dbt::Layered );
	$dbm2->region($image);
	@box = $dbm->clipRect;
	is_deeply(\@box, [0,0,2,2], 'region box on layered');
}

$r2 = $dbm->region;
is_deeply([$r2->box], [0,0,3,3], 'region reused');
$r = $r2;
try('reused check 2',
	"     ".
	"     ".
	"***  ".
	"***  ".
	"***  "
);


$dbm->region( undef );
@box = $dbm->clipRect;
is_deeply(\@box, [0,0,4,4], 'empty clip rect');

$r2 = Prima::Region->new( box => [1, 1, 2, 2]);
$r = $r2->dup;
try('rgnop::Copy',
	"     ".
	"     ".
	" **  ".
	" **  ".
	"     "
);

my $r3 = Prima::Region->new( box => [2, 2, 2, 2]);
$r = $r2->dup;
$r->combine( $r3, rgnop::Union);
try('rgnop::Union',
	"     ".
	"  ** ".
	" *** ".
	" **  ".
	"     "
);

$r = $r2->dup;
$r->combine( $r3, rgnop::Intersect);
try('rgnop::Intersect',
	"     ".
	"     ".
	"  *  ".
	"     ".
	"     "
);

$r = $r2->dup;
$r->combine( $r3, rgnop::Xor);
try('rgnop::Xor',
	"     ".
	"  ** ".
	" * * ".
	" **  ".
	"     "
);

$r = $r2->dup;
$r->combine( $r3, rgnop::Diff);
try('rgnop::Diff',
	"     ".
	"     ".
	" *   ".
	" **  ".
	"     "
);

$r = $dbm->region;
try('region re-reuse',
	"     ".
	"     ".
	" *   ".
	" **  ".
	"     "
);

sub bits  { join ':', map { sprintf "%08b", ord } split '', shift }
sub bytes { unpack('H*', shift ) }

sub is_bytes
{
	my ( $bytes_actual, $bytes_expected, $name ) = @_;
	my $ok = $bytes_actual eq $bytes_expected;
	ok( $ok, $name );
	warn "#   " . bytes($bytes_actual) . " (actual)\n#   " . bytes($bytes_expected) . " (expected)\n" unless $ok;
}

sub sort_boxes($)
{
	my @b = @{$_[0]};
	my (@x,@r);
	push @r, [@x] while @x = splice(@b, 0, 4);
	@r = sort { "@$a" cmp "@$b" } @r;
	return [ map { @$_ } @r];
}

sub is_rects
{
	my ($r, $rects, $name) = @_;
	my $boxes = sort_boxes $r->get_boxes;
	$rects = sort_boxes $rects;
	my $ok = is_deeply( $boxes, $rects, $name);
	warn "# (@$boxes) vs (@$rects)\n" unless $ok;
	return $ok;
}

$r = Prima::Region->new;
$r = Prima::Region->new( rect => [0,1,2,3,4,5,6,7]);
is_rects($r, [0,1,2,2,4,5,2,2], "two simple rects");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (1)");
$r = Prima::Region->new( box => [0,1,2,3,4,5,6,7]);
is_rects($r, [0,1,2,3,4,5,6,7], "two simple boxes");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (2)");
$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0], fillMode => fm::Overlay | fm::Winding);
is_rects($r, [0,0,6,6], "simple polygon");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (3)");

my $b = Prima::Image->new(
	size => [5,5],
	type => im::Byte,
);

sub render
{
	my $rx = shift;
	$b->region(undef);
	$b->color(cl::Black);
	$b->bar(0,0,$b->size);
	$b->color(cl::White);
	$b->region($rx);
	$b->bar(0,0,$b->size);
}

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], fillMode => fm::Winding);
render($r);
is( $b->sum, 25 * 255, "polygon with winding");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (4)");

ok(
	$b->pixel(0,0) != 0 &&
	$b->pixel(1,0) != 0 &&
	$b->pixel(0,1) != 0 &&
	$b->pixel(1,1) != 0,
	"pixels are in correct position (1)"
);

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], fillMode => fm::Alternate);
render($r);
is( $b->sum, 21 * 255, "polygon without winding");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (5)");

ok(
	$b->pixel(0,0) == 0 &&
	$b->pixel(1,0) == 0 &&
	$b->pixel(0,1) == 0 &&
	$b->pixel(1,1) == 0,
	"pixels are in correct position (2)"
);

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], fillMode => fm::Winding|fm::Overlay);
render($r);
is( $b->sum, 25 * 255, "overlay polygon with winding");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (6)");

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], fillMode => fm::Alternate|fm::Overlay);
render($r);
is( $b->sum, 24 * 255, "overlay polygon without winding");
ok( $r->equals( Prima::Region->new(box => $r->get_boxes)), "is equal (7)");

my $d = $b->data;
render($b->to_region);
is_bytes($d, $b->data, "region to image and back is okay");

$b->size(4,4);
$r = Prima::Region->new( box => [-1,-1,3,3, 2,2,3,3]);
render($r);
is_bytes($b->data, ("\xff\xff\x00\x00" x 2).("\x00\x00\xff\xff" x 2), "region outside the box");
$b->color(0x808080);
$b->bar(1,1,2,2);
is_bytes($b->data, 
	"\xff\xff\x00\x00".
	"\xff\x80\x00\x00".
	"\x00\x00\x80\xff".
	"\x00\x00\xff\xff",
	"bar inside region"
);
$b->translate(0,0);
render(undef);
$b->color(cl::Black);
$b->region($r);
$b->translate(1,1);
$b->bar(0,0,$b->size);
is_bytes($b->data, 
	"\xff\xff\xff\xff".
	"\xff\x00\xff\xff".
	"\xff\xff\x00\x00".
	"\xff\xff\x00\x00",
	"region plot with offset 1"
);

$b->translate(0,0);
render(undef);
$b->translate(2,2);
$b->color(cl::Black);
$b->region($r);
$b->bar(0,0,$b->size);
is_bytes($b->data, 
	"\xff\xff\xff\xff".
	"\xff\xff\xff\xff".
	"\xff\xff\x00\x00".
	"\xff\xff\x00\x00",
	"region plot with offset 2"
);

$b->translate(0,0);
render(undef);
$b->translate(5,5);
$b->color(cl::Black);
$b->region($r);
$b->bar(0,0,$b->size);
is( $b->sum, 16 * 255, "region outside left");
$b->translate(-5,-5);
is( $b->sum, 16 * 255, "region outside right");

my $i = Prima::Image->new( size => [32, 32], type => im::Byte);
$i->color(0);
$i->bar(0,0,$i->size);
my $j = $i->dup;
$j->color(cl::White);
$j->bar(0,0,$j->size);
$r = Prima::Region->new( polygon => [0, 0, 10, 25, 25, 0, 0, 15, 25, 15]);
$i->region($r);
$i->put_image(0,0,$j);

my $xr = $r->image(type => im::Byte, size => [32, 32], backColor => 0, color => cl::White);
is( $i->data, $xr->data, 'put_image(region) == region.image');

$i = Prima::Icon->new( size => [32, 32], type => im::Byte, maskType => im::bpp8, autoMasking => am::None);
$i->color(0);
$i->bar(0,0,$i->size);
$j = Prima::Icon->new( size => [32, 32], type => im::Byte, maskType => im::bpp8, autoMasking => am::None);
$j->color(cl::White);
$j->bar(0,0,$j->size);
$j->mask( "\xff" x length($i->mask));
$i->region($r);
$i->put_image(0,0,$j,rop::Blend);
is( $i->data, $xr->data, 'put_image(region).rgb == region.image');
is( $i->mask, $xr->data, 'put_image(region).a8  == region.image');

done_testing;
