use strict;
use warnings;

use Test::More tests => 10;
use Prima::sys::Test qw(noX11);

# noX11 test

my $i = Prima::Image-> create(
       width  => 4,
       height => 2,
       preserveType => 0,
       conversion => ict::None,
       type => im::RGB,
);

my $rgb = "mamAmyLarAmuMamAmyLalAru";
$i-> data( $rgb);
is( $i-> data, $rgb, "import" );
$i-> data( $rgb . reverse $rgb);
is( $i-> data, $rgb, "bounds overset" );
$i-> data( 'M');
substr( $rgb, 0, 1) = 'M';
is( $i-> data, $rgb, "bounds underset" );

my $tester;
$tester = sub {
       my ( $s1, $s2, $format, $test_name) = @_;
       my $new = $rgb;
       eval "\$new =~ s/$s1/$s2/g";
       $i-> set(
               data => $new,
               type => $format,
       );
       is( $i-> data, $rgb, $test_name );
};

# 4
$tester->('([A-Z])([a-z])([a-z])', '$3$2$1',    im::Color|im::bpp24|im::fmtBGR,  "im::fmtBGR" );
$tester->('([A-Z])',               '0$1',       im::Color|im::bpp32|im::fmtIRGB, "im::fmtIRGB" );
$tester->('([A-Z][a-z][a-z])',     '${1}0',     im::Color|im::bpp32|im::fmtRGBI, "im::fmtRGBI" );
$tester->('([A-Z])([a-z])([a-z])', '0$3$2$1',   im::Color|im::bpp32|im::fmtIBGR, "im::fmtIBGR" );
$tester->('([A-Z])([a-z])([a-z])', '$3$2${1}0', im::Color|im::bpp32|im::fmtBGRI, "im::fmtBGRI" );
# 9
$i-> set(
       data    => "\0\1\2\3\4\5\6\7",
       type    => im::bpp8,
       palette => [ map { ord } split('', $rgb)],
);
is( $i-> data, "\0\1\2\3\4\5\6\7", "im::bpp8+palette" );
# 10
$i-> set(
       data    => "\4\5\6\7\0\1\2\3",
       reverse => 1,
       type    => im::bpp8,
);
is( $i-> data, "\0\1\2\3\4\5\6\7", "reverse" );
