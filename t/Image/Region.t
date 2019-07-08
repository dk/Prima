use strict;
use warnings;
use Test::More;
use Prima::Test qw(noX11);

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

my $r = Prima::Region->new;
my $k = $r->get_boxes;
is_deeply( $r->get_boxes, [], 'empty');
$r = Prima::Region->new( rect => [0,1,2,3]);
is_deeply($r->get_boxes, [0,1,2,2], "simple rect");
$r = Prima::Region->new( rect => [0,1,2,3,4,5,6,7]);
is_rects($r, [0,1,2,2,4,5,2,2], "two simple rects");
$r = Prima::Region->new( box => [0,1,2,3]);
is_deeply($r->get_boxes, [0,1,2,3], "simple box");
$r = Prima::Region->new( box => [0,1,2,3,4,5,6,7]);
is_rects($r, [0,1,2,3,4,5,6,7], "two simple boxes");
$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0]);
is_rects($r, [0,0,5,5], "simple polygon");

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

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], winding => 1);
render($r);
is( $b->sum, 25 * 255, "polygon with winding");

$r = Prima::Region->new( polygon => [0,0,0,5,5,5,5,0, 0,0,0,2,2,2,2,0], winding => 0);
render($r);
is( $b->sum, 21 * 255, "polygon without winding");
ok( 
	$b->pixel(0,0) == 0 &&
	$b->pixel(1,0) == 0 &&
	$b->pixel(0,1) == 0 &&
	$b->pixel(1,1) == 0,
	"pixels are in correct position"
);
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
render(undef);
$b->color(cl::Black);
$b->translate(1,1);
$b->region($r);
$b->bar(0,0,$b->size);
is_bytes($b->data, 
	"\x00\x00\x00\xff".
	"\x00\x00\x00\xff".
	"\x00\x00\x00\xff".
	"\xff\xff\xff\x00",
	"region plot with offset 1"
);

render(undef);
$b->translate(2,2);
$b->color(cl::Black);
$b->region($r);
$b->bar(0,0,$b->size);
is_bytes($b->data, 
	"\xff\xff\xff\xff".
	"\xff\x00\x00\x00".
	"\xff\x00\x00\x00".
	"\xff\x00\x00\x00",
	"region plot with offset 2"
);

render(undef);
$b->translate(5,5);
$b->color(cl::Black);
$b->region($r);
$b->bar(0,0,$b->size);
is( $b->sum, 16 * 255, "region outside left");
$b->translate(-5,-5);
is( $b->sum, 16 * 255, "region outside right");

done_testing;
