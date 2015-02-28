use Test::More;

use lib 't/lib';

BEGIN {
    use_ok( "Prima::Test" );
}
if( $Prima::Test::noX11 ) {
    plan skip_all => "Skipping all because noX11";
}

my $a = Prima::Drawable-> create( width => 1, height => 1, type => im::RGB);

$a-> color( 0x123456);
cmp_ok( $a-> color, '==', 0x123456, 'color' );

$a-> backColor( 0x654321);
cmp_ok( $a-> backColor, '==', 0x654321, 'backColor' );

$a-> fillPattern( [0..7]); my $i = 0;
my $fillPatternCount = scalar grep { $i++ != $_ } @{$a-> fillPattern};
cmp_ok( $fillPatternCount, '==', 0, 'fillPattern' );

$a-> lineEnd( le::Square);
cmp_ok( $a-> lineEnd, '==', le::Square, 'lineEnd' );

$a-> lineWidth( 5);
cmp_ok( $a-> lineWidth, '==', 5, 'lineWidth' );

$a-> linePattern( lp::Dash);
is( $a-> linePattern, lp::Dash, 'linePattern' );

$a-> rop( rop::NotSrcXor);
cmp_ok( $a-> rop, '==', rop::NotSrcXor, 'rop' );

$a-> rop2( rop::NotSrcXor);
cmp_ok( $a-> rop2, '==', rop::NotSrcXor, 'rop2' );

$a-> translate( 1, 2);
my @z = $a-> translate;
cmp_ok( $z[0], '==', 1, 'translate' );
cmp_ok( $z[1], '==', 2, 'translate' );

$a-> textOpaque( 1);
cmp_ok( $a-> textOpaque, '==', 1, 'textOpaque' );

$a-> textOutBaseline( 1);
cmp_ok( $a-> textOutBaseline, '==', 1, 'textOutBaseline' );

$a-> lineJoin( lj::Bevel);
cmp_ok( $a-> lineJoin, '==' ,lj::Bevel, "lineJoin");

$a-> fillWinding( 1);
ok( $a-> fillWinding, "fillWinding");

$a-> begin_paint;
$a-> end_paint;
cmp_ok( $a-> color, '==', 0x123456, 'color' );
cmp_ok( $a-> backColor, '==', 0x654321, 'backColor' );

$i = 0;
$fillPatternCount = scalar grep { $i++ != $_ } @{$a-> fillPattern};
cmp_ok( $fillPatternCount, '==', 0, "fillPattern" );
cmp_ok( $a-> lineEnd, '==', le::Square, "lineEnd" );
cmp_ok( $a-> lineWidth, '==', 5, "lineWidth" );
is( $a-> linePattern, lp::Dash, "linePattern" );
cmp_ok( $a-> rop, '==', rop::NotSrcXor, "rop" );
cmp_ok( $a-> rop2, '==' , rop::NotSrcXor, "rop2");

@z = $a-> translate;
cmp_ok( $z[0], '==', 1, "translate" );
cmp_ok( $z[1], '==', 2, "translate" );
cmp_ok( $a-> textOpaque, '==', 1, "textOpaque" );
cmp_ok( $a-> textOutBaseline, '==', 1, "textOutBaseline" );
cmp_ok( $a-> lineJoin, '==', lj::Bevel, "lineJoin" );
ok( $a-> fillWinding, "fillWinding");

done_testing();
