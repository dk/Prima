use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

my $db = Prima::DeviceBitmap->new(width => 1, height => 1);

my $p1 = $db->render_spline([0,0,1,0,1,1], 2);
ok($p1);
is(ref($p1), 'ARRAY');
is(@$p1, 6);
is($p1->[0], 0);
is($p1->[5], 1);
ok(Prima::array::is_array($p1));

Prima::array::append($p1, $p1);
is(@$p1, 12);
ok($db->polyline( $p1));

my $p2 = Prima::array->new_int;
ok(Prima::array::is_array($p2));
is(@$p2, 0);
push @$p2, 4;
Prima::array::append($p2, $p1);
push @$p2, 5;
is($p2->[0], 4);
is($p2->[-1], 5);
is(@$p2, 14);
ok($db->polyline( $p2));

done_testing;
