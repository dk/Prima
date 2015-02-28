# $Id$
print "1..10 import,bounds overset,bounds underset,im::fmtBGR,im::fmtIRGB,im::fmtRGBI,im::fmtIBGR,im::fmtBGRI,im::bpp8+palette,reverse";

my $i = Prima::Image-> create( 
	width  => 4,
	height => 2,
	preserveType => 0,
	conversion => ict::None,
	type => im::RGB,
);

my $rgb = "mamAmyLarAmuMamAmyLalAru";
$i-> data( $rgb);
ok( $i-> data eq $rgb);
$i-> data( $rgb . reverse $rgb);
ok( $i-> data eq $rgb);
$i-> data( 'M');
substr( $rgb, 0, 1) = 'M';
ok( $i-> data eq $rgb);

my $tester;
$tester = sub {
	my ( $s1, $s2, $format) = @_;
	my $new = $rgb;
	eval "\$new =~ s/$s1/$s2/g";
	$i-> set(
		data => $new,
		type => $format,
	);
	ok( $i-> data eq $rgb);
};

# 4
$tester->('([A-Z])([a-z])([a-z])', '$3$2$1',    im::Color|im::bpp24|im::fmtBGR);
$tester->('([A-Z])',               '0$1',       im::Color|im::bpp32|im::fmtIRGB);
$tester->('([A-Z][a-z][a-z])',     '${1}0',     im::Color|im::bpp32|im::fmtRGBI);
$tester->('([A-Z])([a-z])([a-z])', '0$3$2$1',   im::Color|im::bpp32|im::fmtIBGR);
$tester->('([A-Z])([a-z])([a-z])', '$3$2${1}0', im::Color|im::bpp32|im::fmtBGRI);
# 9
$i-> set(
	data    => "\0\1\2\3\4\5\6\7",
	type    => im::bpp8,
	palette => [ map { ord } split('', $rgb)],
);
ok( $i-> data eq "\0\1\2\3\4\5\6\7");
# 10
$i-> set(
	data    => "\4\5\6\7\0\1\2\3",
	reverse => 1,
	type    => im::bpp8,
);
ok( $i-> data eq "\0\1\2\3\4\5\6\7");



1;

