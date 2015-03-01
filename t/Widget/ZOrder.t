use Test::More;
use Prima::Test qw(noX11 w dong);

if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $first   = $Prima::Test::w-> insert( 'Widget');
my $second  = $Prima::Test::w-> insert( 'Widget');

cmp_ok(( $second-> next || 0), '==', $first, "create" );
cmp_ok(( $first-> prev || 0), '==', $second,  "create" );

$Prima::Test::dong = 0;
$second-> set( onZOrderChanged => sub { $Prima::Test::dong = 1; });
$second-> insert_behind( $first);

cmp_ok(( $second-> prev || 0), '==', $first, "runtime" );
cmp_ok(( $first-> next || 0), '==', $second, "runtime" );
ok( $Prima::Test::dong || &Prima::Test::wait, "event" );

$first-> destroy;
$second-> destroy;

done_testing();
