use Test::More;
use Prima::Test qw(noX11);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my @sz = $::application-> size;
cmp_ok( $sz[0], '>', 256, "size");
cmp_ok( $sz[1], '>', 256, "size" );

my @i = $::application-> get_indents;
cmp_ok( $i[0] + $i[2], '<', $sz[0], "indents" );
cmp_ok( $i[1] + $i[3], '<', $sz[1], "indents" );

done_testing();
