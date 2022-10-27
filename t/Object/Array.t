use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

my $db = Prima::DeviceBitmap->new(width => 1, height => 1);

my $p1 = $db->render_spline([0,0,2,0,2,2], precision => 2);
ok($p1);
is(ref($p1), 'ARRAY');
is(@$p1, 8);
is($p1->[0], 0);
is($p1->[7], 2);
ok(Prima::array::is_array($p1));

Prima::array::append($p1, $p1);
is(@$p1, 16);
ok($db->polyline( $p1));

my $p2 = Prima::array->new_int;
ok(Prima::array::is_array($p2));
is(@$p2, 0);
push @$p2, 4;
Prima::array::append($p2, $p1);
push @$p2, 5;
is($p2->[0], 4);
is($p2->[-1], 5);
is(@$p2, 18);
ok($db->polyline( $p2));

$p2 = Prima::array->new_double;
push @$p2, -2.0, -1.9, -1.1, -0.9, -0.5, -0.25, -0.0, 0.0, 0.25, 0.5, 0.9, 1.1, 1.9, 2.0;
my @ref = map { Prima::Utils::floor($_ + .5) } @$p2;
my $tst = Prima::Drawable->render_polyline($p2, integer => 1);
is_deeply(\@ref, $tst);

$p2 = Prima::array->new_int;
push @$p2, 1,2,3,4;
$tst = Prima::Drawable->render_polyline($p2, matrix => [2,0,0,3,0,0], integer => 1);
is_deeply($tst, [2,6,6,12]);

done_testing;
