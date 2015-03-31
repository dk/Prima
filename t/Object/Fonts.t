use strict;
use warnings;

use Test::More;
use Prima::Test;
use Prima::Application;

Prima::options(debug => 'f') if Prima::Application-> get_system_info-> {apc} == apc::Unix;

my $x;

sub t
{
	for ( qw( height size direction)) {
	       my $fx = $x-> font-> $_();
	       $x-> font( $_ => $x-> font-> $_() * 3 + 12);
	       my $fx2 = $x-> font-> $_();
	       SKIP: {
	           if ( $fx2 == $fx) {
	               skip "$_", 1;
	               next;
	           }
	           $x-> font( $_ => $fx);
	           is( $x-> font-> $_(), $fx, "$_");
	       };
	}
	
	# width is a bit special, needs height or size
	for (qw( height size)) {
		my $fh = $x-> font-> $_();
		$x-> font( $_ => $fh ); # size can be fractional inside
		my $fw = $x-> font-> width;
		my $fn = $x-> font-> name;
		$x-> font( $_ => $fh, width => $fw * 3 + 12);
		my $fw2 = $x-> font-> width;
		SKIP: {
		    if ( $fw2 == $fw) {
		        skip "width", 1;
		        next;
		    }
		    $x-> font( $_ => $fh, width => $fw, name => $fn );
		    is( $x-> font-> width, $fw, "width by $_");
		};
	}
	
	
	# style
	my $fx = $x-> font-> style;
	my $newfx = ~$fx;
	$x-> font( style => $newfx);
	my $fx2 = $x-> font-> style;
	SKIP : {
	    if ( $fx2 == $fx) {
	        skip "style", 1;
	    }
	    $x-> font( style => $fx);
	    is( $fx, $x-> font-> style, "style");
	};
	
	# wrapping
	$x-> font-> height( 16);
	my $w = $x-> width;
	my ($a,$b,$c) = @{$x->get_font_abc(ord('e'),ord('e'))};
	SKIP : {
		skip "text wrap", 1 if $a + $b + $c <= 0; # some non-latin or symbol font
		cmp_ok( scalar @{$x-> text_wrap( "ein zwei drei fir funf sechs seben acht neun zehn", $w * 5)}, '>', 4, "text wrap");
	}
}

$x = Prima::DeviceBitmap-> create( monochrome => 1, width => 8, height => 8);
for my $f ( @{$::application->fonts} ) {
	$x->set_font({
		name     => $f->{name},
		height   => $f->{height} || 10,
		pitch    => $f->{pitch},
		encoding => $f->{encoding},
	});
	t;
}
$x-> destroy;

done_testing;
