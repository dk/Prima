print "1..4 import,bounds overset,bounds underset,im::fmtBGR,im::fmtIRGB";

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
$i-> set(
   data => $rgb,
   type => im::Color|im::bpp24|im::fmtBGR,
);
my $bgr = $i-> data;
$i-> set(
   data => $bgr,
   type => im::Color|im::bpp24|im::fmtBGR,
);
ok( $bgr ne $rgb && $i-> data eq $rgb);
my $irgb = $rgb;
$irgb =~ s/([A-Z])/0$1/g;
print STDERR "\n$irgb\n";
$i-> set(
   data => $bgr,
   type => im::Color|im::bpp32|im::fmtIRGB,
);
print STDERR $i-> data;

ok( $i-> data eq $rgb);




1;

