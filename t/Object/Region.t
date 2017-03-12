use strict;
use warnings;

use Test::More;
use Prima::Test;
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
try('empty def',"*" x 25);

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
$r = Prima::Region->new(polygon => \@star, winding => 0);
my @box = $r->box;
ok($box[0] < $box[2] && $box[1] < $box[3], 'star 1');

$r2 = Prima::Region->new(polygon => \@star, winding => 1);
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
@box = $dbm->get_region_box;
is_deeply(\@box, [0,0,3,3], 'region box');
@box = $dbm->clipRect;
is_deeply(\@box, [0,0,2,2], 'region clip rect');

my $dbm2 = Prima::DeviceBitmap->new( size => [5,5], type => dbt::Pixmap );
$dbm2->region($image);
@box = $dbm->get_region_box;
is_deeply(\@box, [0,0,3,3], 'region box on pixmap');

if ( $::application->get_system_value(sv::LayeredWidgets)) {
	$dbm2 = Prima::DeviceBitmap->new( size => [5,5], type => dbt::Layered );
	$dbm2->region($image);
	@box = $dbm->get_region_box;
	is_deeply(\@box, [0,0,3,3], 'region box on layered');
}

$r2 = Prima::Region->new(image => $dbm->region);
is_deeply([$r2->box], [0,0,3,3], 'image reused');

$dbm->region( undef );
@box = $dbm->get_region_box;
is_deeply(\@box, [0,0,5,5], 'empty region box');
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

done_testing;
