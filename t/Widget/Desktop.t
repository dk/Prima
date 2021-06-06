use strict;
use warnings;

use Test::More;
use Prima::sys::Test;
use Prima::Application;

plan tests => 4;

my @sz = $::application-> size;
cmp_ok( $sz[0], '>', 256, "size");
cmp_ok( $sz[1], '>', 256, "size" );

my @i = $::application-> get_indents;
cmp_ok( $i[0] + $i[2], '<', $sz[0], "indents" );
cmp_ok( $i[1] + $i[3], '<', $sz[1], "indents" );
