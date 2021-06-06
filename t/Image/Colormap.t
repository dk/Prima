use strict;
use warnings;

use Test::More tests => 4;
use Prima::sys::Test qw(noX11);

# noX11 test

my $p = [0,1,2,3,4,5];
my $c = [0x020100, 0x050403];
my $i = Prima::Image-> create(
       width => 1,
       height => 1,
       type => im::Mono,
       colormap => $c,
);

# 1
is_deeply( $i-> palette, $p, "create" );

# 2
is_deeply( [ $i-> colormap ], $c, "get" );

# 3
$i-> colormap( @$c);
is_deeply( $i-> palette, $p, "set" );

# 4
my $cc = $i-> get_nearest_color(0x030101);
is( $cc, 0x020100, "nearest" );
