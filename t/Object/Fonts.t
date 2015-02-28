# $Id$
print "1..6 height,width,size,direction,pitch,style,text_wrap\n";

my $x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);

for ( qw( height width size direction)) {
	my $fx = $x-> font-> $_();
	$x-> font( $_ => $x-> font-> $_() * 3 + 12);
	my $fx2 = $x-> font-> $_();
	if ( $fx2 == $fx) {
		skip;
	} else {
		$x-> font( $_ => $fx);
		ok( $fx == $x-> font-> $_());
	}
}

my $fx = $x-> font-> pitch;
my $newfx = ( $fx == fp::Fixed) ? fp::Variable : fp::Fixed;
$x-> font( pitch => $newfx);
my $fx2 = $x-> font-> pitch;
$x-> font( pitch => $fx);
ok( $x-> font-> pitch == $fx && $fx2 == $newfx);

$fx = $x-> font-> style;
$newfx = ~$fx;
$x-> font( style => $newfx);
$fx2 = $x-> font-> style;
if ( $fx2 == $fx) {
	print "ok # skip";
} else {
	$x-> font( style => $fx);
	ok( $fx == $x-> font-> style);
}

$x-> font-> height( 16);
my $w = $x-> width;
ok( scalar @{$x-> text_wrap( "Ein zwei drei fir funf sechs seben acht neun zehn", $w * 5)} > 4);

$x-> destroy;
1;
