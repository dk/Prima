use strict;
use warnings;
use Prima::PS::Printer;
use Prima::PS::PDF;
use Prima::PS::PostScript;
use Encode;

sub page
{
	my ($p, $name) = @_;
	my @size = $p->size;
	$p->translate(0,0);
	$p-> font-> set( size=>100,name=>$name);
	my $h100 = $p->font->height;

	my $m = $p-> get_font;
	my $xtext = Encode::decode('latin1', "\x{c5}Mg");

	my $glyphs = $p-> text_shape( $xtext, level => ts::Glyphs, polyfont => 0 );
	if ( $glyphs && grep { $_ == 0 } @{ $glyphs->glyphs }) {
		# bad glyphs find some others (best if not ascii)
		my @g;
		$xtext = '';
		my $r = $p-> get_font_ranges;
		GLYPHS: for ( my $i = @$r - 2; $i >= 0; $i -= 2) {
			my ( $from, $to ) = @{$r}[$i,$i+1];
			for ( my $j = $from; $j <= $to; $j++) {
				$xtext .= chr($j);
				last GLYPHS if length($xtext) > 2;
			}
		}
	}

	my $s = $size[1] - $m-> {height} - $m-> {externalLeading} - 220;
	my $w = $p-> get_text_width($xtext) + 66;
	$p-> textOutBaseline(1);
	$p-> text_out($xtext, 20, $s);

	my $cachedFacename = $p-> font-> name;
	my $hsf = $p-> font-> height / 6;
	$hsf = 10 if $hsf < 10;
	$p-> font-> set(
		height   => $hsf,
		style    => fs::Italic,
		name     => '',
		encoding => '',
	);

	$p-> line( 2, $s, $w, $s);
	$p-> textOutBaseline(0);
	$p-> text_out( "Baseline", $w - 8, $s);
	my $sd = $s - $m-> {descent};
	$p-> line( 2, $sd, $w, $sd);
	$p-> text_out( "Descent",  $w - 8, $sd);
	$sd = $s + $m-> {ascent};
	$p-> line( 2, $sd, $w, $sd);
	$p-> text_out( "Ascent",  $w - 8, $sd);
	$sd = $s + $m-> {ascent} + $m-> {externalLeading};

	if ( $m-> {ascent} > 4) {
		$p-> line( $w - 12, $s + 1, $w - 12, $s + $m-> {ascent});
		$p-> line( $w - 12, $s + $m-> {ascent}, $w - 14, $s + $m-> {ascent} - 2);
		$p-> line( $w - 12, $s + $m-> {ascent}, $w - 10, $s + $m-> {ascent} - 2);
	}
	if ( ($m-> {ascent}-$m-> {internalLeading}) > 4) {
		my $pt = $m-> {ascent}-$m-> {internalLeading};
		$p-> line( $w - 16, $s + 1, $w - 16, $s + $pt);
		$p-> line( $w - 16, $s + $pt, $w - 18, $s + $pt - 2);
		$p-> line( $w - 16, $s + $pt, $w - 14, $s + $pt - 2);
	}
	if ( $m-> {descent} > 4) {
		$p-> line( $w - 13, $s - 1, $w - 13, $s - $m-> {descent});
		$p-> line( $w - 13, $s - $m-> {descent}, $w - 15, $s - $m-> {descent} + 2);
		$p-> line( $w - 13, $s - $m-> {descent}, $w - 11, $s - $m-> {descent} + 2)
	}

	my $str;
	$p-> text_out( "External Leading",  2, $sd);
	$p-> line( 2, $sd, $w, $sd);
	$sd = $s + $m-> {ascent} - $m-> {internalLeading};
	$str = "Point size in device units";
	$p-> text_out( $str,  $w - 16 - $p-> get_text_width( $str), $sd);
	$p-> linePattern( lp::Dash);
	$p-> line( 2, $sd, $w, $sd);

	$p-> font-> set(
		size => 10,
		pitch  => fp::Fixed,
	);
	my $fh = $p-> font-> height;
	$sd = $s - $m-> {descent} - $fh * 3;
	$p-> text_out( 'nominal size        : '.$m-> {size}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'cell height         : '.$m-> {height   }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'average width       : '.$m-> {width    }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'ascent              : '.$m-> {ascent   }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'descent             : '.$m-> {descent  }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'weight              : '.$m-> {weight   }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'internal leading    : '.$m-> {internalLeading}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'external leading    : '.$m-> {externalLeading}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'maximal width       : '.$m-> {maximalWidth}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'horizontal dev.res. : '.$m-> {xDeviceRes}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'vertical dev.res.   : '.$m-> {yDeviceRes}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'first char          : '.$m-> {firstChar}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'last char           : '.$m-> {lastChar }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'break char          : '.$m-> {breakChar}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'default char        : '.$m-> {defaultChar}, 2, $sd); $sd -= $fh;
	$p-> text_out( 'family              : '.$m-> {family   }, 2, $sd); $sd -= $fh;
	$p-> text_out( 'face name           : '.$cachedFacename, 2, $sd); $sd -= $fh;

	my $C = 'f';

	$sd -= $h100 - 20;
	#delete $m->{height};
	#delete $m->{width};
	$p->font( name => $m->{name}, size => $m->{size}, style=> $m->{style} );
	$p-> line(50, $sd+$p->font->descent, 1000, $sd+$p->font->descent);
	$p-> linePattern(lp::Dot);
	my $ddx = 0;
	for my $C ( split //, $xtext ) {
		my $flags = utf8::is_utf8($C) ? to::Unicode : 0;
		my ( $a, $b, $c ) = @{ $p->get_font_abc( ord($C), ord($C), $flags) };
		my ( $d, $e, $f ) = @{ $p->get_font_def( ord($C), ord($C), $flags) };

		my $w = (( $a < 0 ) ? 0 : $a) + $b + (( $c < 0 ) ? 0 : $c);
		my $h = (( $d < 0 ) ? 0 : $d) + $e + (( $f < 0 ) ? 0 : $f);
		# print ord($C), "/$C: $a $b $c ($w) / $d $e $f \n";
		$h = $sd;
		$p-> translate(350 + $ddx, $h);
		$ddx += $w + 20;
		$w = 50;

		my $dx = 0;
		my $dy = 0;
		$dx -= $a if $a < 0;
		$dy -= $d if $d < 0;

		my $fh = $p-> font->height;
		$p-> text_out( $C, $dx, $dy );

		$dx = abs($a);
		$dy = abs($d);
		$p-> line($dx, 0, $dx, $m->{height});
		$dx = (( $a < 0 ) ? 0 : $a) + $b + (( $c < 0 ) ? 0 : $c) - abs($c);
		$p-> line($dx, 0, $dx, $m->{height});

		$p-> line(0, $dy, $m->{width}, $dy);
		$dy = (( $d < 0 ) ? 0 : $d) + $e + (( $f < 0 ) ? 0 : $f) - abs($f);
		$p-> line(0, $dy, $m->{width}, $dy);
	}
}

if (!@ARGV || $ARGV[0] !~ /^\-(ps|pdf)$/) {
	print "Please run with either -ps or -pdf\n";
	exit(1);
}

my $p = ($ARGV[0] eq '-ps') ?
	Prima::PS::File->new( file => 'out.ps') :
	Prima::PS::PDF::File->new( file => 'out.pdf');

$p->begin_doc;
my $ff = $p->font;
my @fonts = @{$p-> fonts};
my $i;
$|++;
for my $f ( sort { $a->{name} cmp $b->{name} } @fonts ) {
	$i++;
	printf "[%d/%d] %s              \r", $i, scalar(@fonts), $f->{name};
	$p->font($ff);
	page( $p, $f->{name} );
	$p->new_page;
}

$p->end_doc;
print "\n", $p->file, " generated ok\n";
