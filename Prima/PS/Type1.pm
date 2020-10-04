package Prima::PS::Type1;

use strict;
use warnings;
use Prima::PS::Glyphs;
use Prima::PS::TempFile;
use Prima::PS::Unicode;
use Prima::Utils;
use base qw(Prima::PS::Glyphs);

sub create_font_entry
{
	my ( $self, $key, $font ) = @_;

	my %h;
	$h{isFixedPitch} = ($font->{pitch} == fp::Fixed) ? 'true'      : 'false';
	$h{Weight}       = ($font->{style} & fs::Bold)   ? '(Regular)' : '(Bold)';
	$h{ItalicAngle}  = ($font->{style} & fs::Italic) ? '-10'       : '0';

	return {
		glyphs   => '',
		chars    => '',
		header   => \%h,
		bbox     => [ undef, undef, undef, undef ],
		scale    => ($font->{height} - $font->{internalLeading}) / $font->{size},
	};
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
			goto STD unless exists $Prima::PS::Unicode->{ $suggested_gid };
			$f->{$suggested_gid} = $charid;
		}
		$glyphid = $Prima::PS::Unicode->{ $suggested_gid };
		$vector = 'chars';
	} else {
	STD:
		$glyphid = sprintf("g%x", $charid);
	}
	return $glyphid if vec($f->{$vector}, $charid, 1);

	vec($f->{$vector}, $charid, 1) = 1;
	$f->{tmpfile} //= Prima::PS::TempFile->new;
	my ($code) = $self->get_outline( $canvas, $key, $charid, 1 );
	$f->{tmpfile}->write("/$glyphid " .embed2($code)) if defined $code;

	return $glyphid;
}

1;

=pod

=head1 NAME

Prima::PS::Type1 - create Type1 font files

=head1 DESCRIPTION

This module contains helper procedures to store Type1 fonts.

=cut
