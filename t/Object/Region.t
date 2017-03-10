use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

my $dbm = Prima::DeviceBitmap->new( size => [5, 5], monochrome => 1 );
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
try('empty def',"*" x 25);

# rect
$r = Prima::Region->new( box => [0, 0, 1, 1]);
try('box 1x1',
	"     ".
	"     ".
	"     ".
	"     ".
	"*    "
);
$r = Prima::Region->new( box => [1, 1, 3, 3]);
try('box 3x3',
	"     ".
	" *** ".
	" *** ".
	" *** ".
	"     "
);
my $r2 = Prima::Region->new( rect => [1, 1, 4, 4]);
ok( $r->equals($r2), 'equals');

$r = Prima::Region->new( ellipse => [ 3, 3, 1, 1]);
is_deeply([$r->box], [3,3,1,1], 'ellipse 1x1');

$r = Prima::Region->new( ellipse => [ 3, 3, 2, 2]);
is_deeply([$r->box], [3,3,2,2], 'ellipse 2x2');

$r = Prima::Region->new( ellipse => [ 3, 3, 3, 3]);
is_deeply([$r->box], [2,2,3,3], 'ellipse 3x3');

$r = Prima::Region->new( ellipse => [ 3, 3, 4, 4]);
is_deeply([$r->box], [2,2,4,4], 'ellipse 4x4');

$r = Prima::Region->new( ellipse => [ 3, 3, 5, 5]);
is_deeply([$r->box], [1,1,5,5], 'ellipse 5x5');

$r = Prima::Region->new( ellipse => [ 3, 3, 6, 6]);
is_deeply([$r->box], [1,1,6,6], 'ellipse 6x6');

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
		"\x00\x00\x00\x00".
		"\x00\x00\x00\x00"  # y=3
);
$r = Prima::Region->new(image => $image);
is_deeply([$r->box], [0,0,3,2], 'image');

#$dbm->region($image);
#$dbm->region->save('1.bmp');
#$r2 = Prima::Region->new(image => $dbm->region);
#ok($r->equals($r2), 'set/get region');

done_testing;
