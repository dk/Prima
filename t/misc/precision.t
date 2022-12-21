use strict;
use warnings;
use Test::More;
use Prima::sys::Test qw(noX11);
use Prima::Utils qw(nearest_d nearest_i);

is( nearest_i( -1.8 ), -2, "nearest(-1.8) == -2");
is( nearest_i( -1.6 ), -2, "nearest(-1.6) == -2");
is( nearest_i( -1.50001 ), -2, "nearest(-1.50001) == -2");
is( nearest_i( -1.49999 ), -1, "nearest(-1.49999) == -1");
is( nearest_i( -1.4 ), -1, "nearest(-1.4) == -1");
is( nearest_i( -1.1 ), -1, "nearest(-1.1) == -1");
is( nearest_i( -1.0 ), -1, "nearest(-1.1) == -1");
is( nearest_i( -0.9 ), -1, "nearest(-0.9) == -1");
is( nearest_i( -0.50001 ), -1, "nearest(-0.50001) == -1");
is( nearest_i( -0.49999 ), 0, "nearest(-0.49999) == 0");
is( nearest_i( -0.4 ), -0, "nearest(-0.4) == -0");
is( nearest_i( -0.1 ), -0, "nearest(-0.1) == -0");
is( nearest_i( 0.0 ), -0, "nearest(-0.1) == -0");

is( nearest_i( 1.8 ), 2, "nearest(1.8) == 2");
is( nearest_i( 1.6 ), 2, "nearest(1.6) == 2");
is( nearest_i( 1.50001 ), 2, "nearest(1.50001) == 2");
is( nearest_i( 1.49999 ), 1, "nearest(1.49999) == 1");
is( nearest_i( 1.4 ), 1, "nearest(1.4) == 1");
is( nearest_i( 1.1 ), 1, "nearest(1.1) == 1");
is( nearest_i( 1.0 ), 1, "nearest(1.1) == 1");
is( nearest_i( 0.9 ), 1, "nearest(0.9) == 1");
is( nearest_i( 0.50001 ), 1, "nearest(0.50001) == 1");
is( nearest_i( 0.49999 ), 0, "nearest(0.49999) == 0");
is( nearest_i( 0.4 ), 0, "nearest(0.4) == 0");
is( nearest_i( 0.1 ), 0, "nearest(0.1) == 0");

sub isd($) { is( nearest_d(int($_[0] * 1e6) / 1e6), int($_[0] * 1e6) / 1e6, "nearest_d($_[0])" ) }

isd( -1.8 );
isd( -1.6 );
isd( -1.50001 );
isd( -1.49999 );
isd( -1.4 );
isd( -1.1 );
isd( -1.0 );
isd( -0.9 );
isd( -0.50001 );
isd( -0.49999 );
isd( -0.4);
isd( -0.1);
isd( -0.0);
isd( 1.8 );
isd( 1.6 );
isd( 1.50001 );
isd( 1.49999 );
isd( 1.4 );
isd( 1.1 );
isd( 1.0 );
isd( 0.9 );
isd( 0.50001 );
isd( 0.49999 );
isd( 0.4);
isd( 0.1);
isd( 0.0);

is_deeply( [nearest_i(1,2)], [1,2], "2i");
is_deeply( [nearest_i(1,2,3)], [1,2,3], "3i");
is_deeply( [nearest_i(1,2,3,4)], [1,2,3,4], "4i");
is_deeply( nearest_i([1,2]), [1,2], "2ri");
is_deeply( nearest_i([1,2,3]), [1,2,3], "3ri");
is_deeply( nearest_i([1,2,3,4]), [1,2,3,4], "4ri");

is_deeply( [nearest_d(1,2)], [1,2], "2d");
is_deeply( [nearest_d(1,2,3)], [1,2,3], "3d");
is_deeply( [nearest_d(1,2,3,4)], [1,2,3,4], "4d");
is_deeply( nearest_d([1,2]), [1,2], "2rd");
is_deeply( nearest_d([1,2,3]), [1,2,3], "3rd");
is_deeply( nearest_d([1,2,3,4]), [1,2,3,4], "4rd");

done_testing;
