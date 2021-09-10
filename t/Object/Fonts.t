use strict;
use warnings;
use open ':std', ':encoding(utf8)';

use Test::More;
use Prima::sys::Test;
use Prima::Application;
use open ':std', ':encoding(utf8)';

my $x;

sub t
{
	my $f = shift;

	$x->set_font({
		name     => $f->{name},
		height   => $f->{height} || 10,
		pitch    => $f->{pitch},
		encoding => $f->{encoding},
	});

	my $ok = 1;
	for ( qw( height size direction)) {
	       my $fx = $x-> font-> $_();
	       $x-> font( $_ => $x-> font-> $_() * 3 + 12);
	       my $fx2 = $x-> font-> $_();
	       SKIP: {
	           if ( $fx2 == $fx || $x->font->name ne $f->{name}) {
	               skip "$_", 1;
	               next;
	           }
	           $x-> font( $_ => $fx);
	           $ok &= is( $x-> font-> $_(), $fx, "$_ / $f->{name}");
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
		    if ( $fw2 == $fw || $x->font->name ne $f->{name}) {
		        skip "width / $f->{name}", 1;
		        next;
		    }
		    $x-> font( $_ => $fh, width => $fw, name => $fn );
		    $ok &= is( $x-> font-> width, $fw, "width by $_ / $f->{name}");
		};
	}


	# style
	my $fx = $x-> font-> style;
	my $newfx = ~$fx;
	$x-> font( style => $newfx);
	my $fx2 = $x-> font-> style;
	SKIP : {
	    if ( $fx2 == $fx || $x->font->name ne $f->{name}) {
	        skip "style", 1;
	    }
	    $x-> font( style => $fx);
	    is( $x-> font-> style, $fx, "style / $f->{name}");
	};

	# wrapping
	SKIP : {
		$x-> font-> height( 16);
		my ($a,$b,$c) = @{$x->get_font_abc(ord('e'),ord('e'))};
		my $w = $a + $b + $c;
		skip "text wrap $f->{name}", 1 if $w <= 1; # some non-latin or symbol font
		cmp_ok( scalar @{$x-> text_wrap( "eeee eeee eeee eeee eeee", $w * 5)}, '>', 4, "text wrap $f->{name}");
	}

	return $ok;
}

my $filter = @ARGV ? qr/$ARGV[0]/ : qr/./;
my $bad_guys = qr/(
	Color\sEmoji|     # fontconfig doesn't support this .ttc, reports crazy numbers and cannot display it
	Pebble            # no glyphs in the font?
)/x;

$x = Prima::DeviceBitmap-> create( type => dbt::Bitmap, width => 8, height => 8);
my @fonts;
for my $f ( @{$::application->fonts} ) {
	next if $f->{name} =~ /$bad_guys/;
	next unless $f->{name} =~ /$filter/;
	push @fonts, $f;
}

plan tests => scalar(@fonts) * 7;

for my $f ( @fonts ) {
	if (!t($f) && Prima::Application-> get_system_info-> {apc} == apc::Unix) {
		Prima::options(debug => 'f');
		t($f);
		Prima::options(debug => '0');
	}
}
$x-> destroy;

done_testing;
