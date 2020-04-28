package Prima::PS::Glyphs;

use strict;
use warnings;
use Prima::PS::Encodings;
use Prima::PS::TempFile;
use Prima::Utils;

sub new
{
	return bless {
		fonts     => {},
	}, shift;
}

sub _create_font_entry
{
	my ( $key, $font ) = @_;

	my %h;
	$h{isFixedPitch} = ($font->{pitch} == fp::Fixed) ? 'true'      : 'false';
	$h{Weight}       = ($font->{style} & fs::Bold)   ? '(Regular)' : '(Bold)';
	$h{ItalicAngle}  = ($font->{style} & fs::Italic) ? '-10'       : '0';

	return {
		glyphs   => '',
		chars    => '',
		header   => \%h,
		names    => {},
		bbox     => [ undef, undef, undef, undef ],
		scale    => ($font->{height} - $font->{internalLeading}) / $font->{size},
	};
}

sub get_font
{
	my ( $self, $font ) = @_;

	my $key = 'PS-' . $font->{name};
	$key .= '-Bold'   if $font->{style} & fs::Bold;
	$key .= '-Italic' if $font->{style} & fs::Italic;
	$key =~ s/\s+/-/g;
	$self->{fonts}->{$key} //= _create_font_entry($key, $font);
	return $key;
}

my $C1       = 52845;
my $C2       = 22719;
my $ENCRYPT1 = 55665;
my $ENCRYPT2 =  4330;
my @HEX      = ('0'..'9','a'..'f');

sub encrypt1
{
	my ( $R, $str ) = @_;
	my $ret = '';
	my $n = 0;
	for ( map { ord } split //, $str ) {
		$n++;
		my $c = $_ ^ ( $$R >> 8 );
		$$R = (($c + $$R) * $C1 + $C2) & 0xffff;
		$ret .= $HEX[$c >> 4];
		$ret .= $HEX[$c & 0xf];
		$ret .= "\n" unless $n % 32;
	}
	return $ret . "\n";
}

sub encrypt2
{
	my $str = shift;
	my $R   = $ENCRYPT2;
	my $ret = '';
	for ( 0,0,0,0, map { ord } split //, $str ) {
		my $c = $_ ^ ( $R >> 8 );
		$R = (($c + $R) * $C1 + $C2) & 0xffff;
		$ret .= chr($c);
	}
	return $ret;
}

sub embed($)
{
	my $code = shift;
	return (4 + length($code)) . ' -| ' . encrypt2($code) . " |\n";
}

sub embed2($)
{
	my $code = shift;
	return (4 + length($code)) . ' -| ' . encrypt2($code) . " |-\n";
}

sub int32($)
{
	my $n = Prima::Utils::floor( $_[0] + .5 );
	if (-107 <= $n && $n <= 107) {
		return chr($n + 139);
	} elsif (108 <= $n && $n <= 1131) {
		$n -= 108;
		return chr(($n >> 8) + 247).chr($n & 0xff);
	} elsif (-1131 <= $n && $n <= -108) {
		$n = -$n - 108;
		return chr(($n >> 8) + 251).chr($n & 0xff);
	} else {
		return chr(255).chr(($n >> 24) & 0xff).chr(($n >> 16) & 0xff).chr(($n >> 8) & 0xff).chr($n & 0xff);
	}
}

sub num { join '', map { int32 $_ } @_ }

use constant closepath       => "\x{9}";
use constant endchar         => "\x{e}";
use constant xpop            => "\x{c}\x{11}";
use constant xreturn         => "\x{b}";
use constant setcurrentpoint => "\x{c}\x{21}";
use constant callothersubr   => "\x{c}\x{10}";
use constant callsubr        => "\x{a}";

sub evacuate
{
	my ( $self, $emit ) = @_;
	for my $fn ( sort keys %{ $self->{fonts} }) {
		my $v = $self->{fonts}->{$fn};
		next unless $v->{tmpfile};

		my $h = $v->{header};

		$emit->(<<FONT_HDR);
%%BeginResource: font $fn
12 dict dup begin
/FontType 1 def
/FontName /$fn def
/FullName ($fn) def
/FontInfo 13 dict dup begin
/UnderlinePosition -100 def
/UnderlineThickness 50 def
FONT_HDR
		$emit->("/$_ $h->{$_} def\n") for sort keys %$h;
		my @bbox = map { Prima::Utils::floor(($_ // 0) + .5) } @{ $v->{bbox} };
		$emit->(<<FONT_HDR2);
end def
/FontBBox {@bbox} def
/PaintType 0 def
/FontMatrix [0.001 0 0 0.001 0 0] def
/Encoding StandardEncoding def
end
currentfile eexec
FONT_HDR2

		my $R = $ENCRYPT1;
		$emit->(encrypt1(\$R, <<GLYPHS_HDR));
\0\0\0\0 dup /Private
13 dict dup begin
/-| {string currentfile exch readstring pop} def
/|- {def} def
/| {put} def
/BlueValues [$bbox[1] 0] def
/password 5839 def
/MinFeature {16 16} def
/OtherSubrs[{}{}{}{systemdict/internaldict known not{pop 3}{1183615869
systemdict/internaldict get exec dup/startlock known{/startlock get exec}{dup
/strtlck known{/strtlck get exec}{pop 3}ifelse}ifelse}ifelse}executeonly]def
/Subrs 5 array
GLYPHS_HDR
		my $subrs =
			"dup 0 " . embed(num(3,0) . callothersubr . xpop . xpop . setcurrentpoint . xreturn ) .
			"dup 1 " . embed(num(0,1) . callothersubr . xreturn ) .
			"dup 2 " . embed(num(0,2) . callothersubr . xreturn ) .
			"dup 3 " . embed( xreturn ) .
			"dup 4 " . embed(num(3,1,3) . callothersubr . xpop . callsubr . xreturn ) .
			"def put dup /CharStrings 257 dict dup begin" .
			"/.notdef " . embed2( hsbw(0,0) . endchar )
			;
		$emit->(encrypt1(\$R, $subrs));
		return 0 unless $v->{tmpfile}->evacuate(sub { $emit->(encrypt1(\$R, $_[0])) });
		$emit->(encrypt1(\$R, <<GLYPHS_FOOTER));
end put
end
dup /FontName get exch definefont pop
mark
currentfile closefile
GLYPHS_FOOTER
		$emit->(("0" x 64) . "\n") for 1..8;
		$emit->(<<RESOURCE_END) or return 0;
cleartomark
%%EndResource

RESOURCE_END
	}

	return 1;
}

sub conic2curve
{
	my ($x0, $y0, $x1, $y1, $x2, $y2) = @_;
	my (@cp1, @cp2);
	$cp1[0] = $x0 + 2 / 3 * ($x1 - $x0);
	$cp1[1] = $y0 + 2 / 3 * ($y1 - $y0);
	$cp2[0] = $x2 + 2 / 3 * ($x1 - $x2);
	$cp2[1] = $y2 + 2 / 3 * ($y1 - $y2);
	return $x0, $y0, @cp1, @cp2, $x2, $y2;
}

sub rrcurveto
{
	my ($x0, $y0, @rest) = @_;
	my @out;
	for ( my $i = 0; $i < @rest; $i += 2 ) {
		my @p = @rest[$i,$i+1];
		$rest[$i]   -= $x0;
		$rest[$i+1] -= $y0;
		push @out, @rest[$i,$i+1];
		($x0, $y0) = @p;
	}
	return num(@out) . "\x{8}";
}

sub hsbw    { num(@_) . "\x{0d}" }
sub rmoveto { num(@_) . "\x{15}" }
sub rlineto { num(@_) . "\x{05}" }
sub hmoveto { num(@_) . "\x{16}" }

my $unicode_glyph_names;

sub use_char
{
	my ( $self, $canvas, $key, $charid, $suggested_gid) = @_;
	my $f = $self->{fonts}->{$key} // return;

	my $glyphid;
	my $vector = 'glyphs';
	if ( defined($suggested_gid)) {
		if ( exists $f->{$suggested_gid} ) {
			goto STD if $f->{$suggested_gid} != $charid;
		} else {
			$unicode_glyph_names //= Prima::PS::Encodings::load_unicode;
			goto STD unless exists $unicode_glyph_names->{ $suggested_gid };
			$f->{$suggested_gid} = $charid;
		}
		$glyphid = $unicode_glyph_names->{ $suggested_gid };
		$vector = 'chars';
	} else {
	STD:
		$glyphid = sprintf("uni%x", $charid);
	}
	return $glyphid if vec($f->{$vector}, $charid, 1);

	my $outline = $canvas->render_glyph($charid, glyph => 1) or return;

	vec($f->{$vector}, $charid, 1) = 1;
	$f->{tmpfile} //= Prima::PS::TempFile->new;

	my @abc  = map { $_ / $f->{scale} } @{$canvas-> get_font_abc(($charid) x 2, to::Glyphs)};
	my @hsbw = ($abc[0], $abc[0] + $abc[1] + $abc[2]);
	my $bbox = $f->{bbox};

	my $size = scalar(@$outline);
	my $first_move;

	my $code = hsbw(@hsbw);
	if ( $size && $outline->[0] != ggo::Move ) {
		$code .= hmoveto($hsbw[0]);
	} else {
		$first_move = $hsbw[0];
	}

	my $closed;
	my @p = (0,0);
	my $scale = $f->{scale} * 64;
	for ( my $i = 0; $i < $size; ) {
		my $cmd = $outline->[$i++];
		my $pts = $outline->[$i++] * 2;

		my $fill_bbox = $i == 2 && !defined $bbox->[0];
		my @pts = map { $outline->[$i++] / $scale } 0 .. $pts - 1;
		if ( $fill_bbox ) {
			$$bbox[0] = $$bbox[2] = $pts[0];
			$$bbox[1] = $$bbox[3] = $pts[1];
		}

		for ( my $j = 0; $j < @pts; $j += 2 ) {
			my ( $x, $y ) = @pts[$j,$j+1];
			$$bbox[0] = $x if $$bbox[0] > $x;
			$$bbox[1] = $y if $$bbox[1] > $y;
			$$bbox[2] = $x if $$bbox[2] < $x;
			$$bbox[3] = $y if $$bbox[3] < $y;
		}

		if ( $cmd == ggo::Move ) {
			$code .= closepath if $closed++;
			my @r = ($pts[0] - $p[0], $pts[1] - $p[1]);
			if ( defined $first_move ) {
				$r[0] -= $first_move;
				undef $first_move;
			}
			$code .= rmoveto(@r);
		} elsif ( $cmd == ggo::Line ) {
			for ( my $j = 0; $j < @pts; $j += 2 ) {
				my @r = ($pts[$j] - $p[0], $pts[$j + 1] - $p[1]);
				@p = @pts[$j,$j+1];
				$code .= rlineto(@r);
			}
		} elsif ( $cmd == ggo::Conic ) {
			if ( @pts == 4 ) {
				$code .= rrcurveto(conic2curve(@p, @pts));
			} else {
				my @xts = (@p, @pts[0,1]);
				for ( my $j = 0; $j < @pts - 4; $j += 2 ) {
					push @xts,
						($pts[$j + 2] + $pts[$j + 0]) / 2,
						($pts[$j + 3] + $pts[$j + 1]) / 2,
						$pts[$j + 2], $pts[$j + 3],
				}
				push @xts, @pts[-2,-1];
				for ( my $j = 0; $j < @xts - 4; $j += 4) {
					$code .= rrcurveto(conic2curve(@xts[$j .. $j + 5]));
				}
			}
		} elsif ( $cmd == ggo::Cubic ) {
			$code .= rrcurveto(@p, @pts);
		}
		@p = @pts[-2,-1];
	}
	$code .= closepath . endchar;

	$f->{tmpfile}->write("/$glyphid " .embed2($code));
	return $glyphid;
}

1;
