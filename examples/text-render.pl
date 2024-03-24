=head1 NAME

examples/text-render.pl - render fonts in terminal

=head1 SYNOPSIS

  $ perl examples/text-render.pl --ascii 12.BI.Arial foo

    vvv
  vXX^^
 XXXXX vXXXXv   vXXXXv
  XX  vX^   XX vX^   XX
  XX  XX    XX XX    XX
 XX   ^XvvvXX  ^XvvvXX
 ^^     ^^^^     ^^^^

=cut

use strict;
use warnings;
use utf8;
use Prima::noARGV;
use Prima::noX11;
use Prima;
use Getopt::Long;

my %opt = (
	help      => 0,
	shaping   => 2,
	direction => 0,
	box       => 0,
	polyfont  => 1,
	rtl       => 'default',
	lang      => 'default',
	pitch     => 'default',
	ascii     => 0,
);

sub usage
{
	print <<USAGE;

$0 - render fonts in terminal

format:
	render-text [options] font-name text [text...]

* font-name: [size=12.][BIOUTS.]name
* use x{04A0} as a unicode codepoint

options:
       --shaping=0-3     use shaping (0-none, 1-glyphs, 2-full, 3-bytes)
       --direction=ANGLE
       --box             draw text inside a box
       --polyfont=1
       --rtl=0|1|default
       --lang=default
       --pitch=default|variable|fixed
       --ascii           poor man's block characters

USAGE
	exit 1;
}

GetOptions(\%opt,
	"shaping=i",
	"direction=i",
	"ascii",
	"box",
	"polyfont=i",
	"rtl=s",
	"lang=s",
	"pitch=s",
) or usage;
usage if $opt{help};
usage if $opt{shaping} > 3;
usage if $opt{rtl} !~ /^(0|1|default)$/;
usage if $opt{pitch} !~ /^(variable|fixed|default)$/;
usage if @ARGV < 2;
my %font;
if ( $ARGV[0] =~ m/^(\d+)\.(.+)$/) {
	%font = ( size => $1, name => $2 );
} else {
	%font = ( name => $ARGV[0], size => 12 );
}
$font{direction} = $opt{direction};
shift @ARGV;
if ( $font{name} =~ m/^([BIOUTS]+)\.(.+)$/) {
	$font{name} = $2;
	$font{style} = 0;
	for ( split //, $1 ) {
		$font{style} |= fs::Bold       if $_ eq 'B';
		$font{style} |= fs::Italic     if $_ eq 'I';
		$font{style} |= fs::Thin       if $_ eq 'T';
		$font{style} |= fs::StruckOut  if $_ eq 'S';
		$font{style} |= fs::Underlined if $_ eq 'U';
		$font{style} |= fs::Outline    if $_ eq 'O';
	}
}
$font{pitch} = fp::Variable if $opt{pitch} eq 'variable';
$font{pitch} = fp::Fixed if $opt{pitch} eq 'fixed';

my $text = join(' ', @ARGV);
$text =~ s/x\{([A-F0-9]+)\}/chr(hex($1))/gei;

my $i = Prima::Image->new(
	size    => [1,1],
	type    => im::BW,
	font    => \%font,
	scaling => ist::None,
	textOutBaseline => 1,
);
if ( $opt{box}) {
	$i->set(
		textOpaque => 1,
		color => 0xffffff,
		backColor => 0,
	);
}
if ( $opt{shaping} ) {
	my %xopt = (
		level    => $opt{shaping},
		polyfont => $opt{polyfont},
		advances => 1,
	);
	$xopt{rtl} = $opt{rtl} if $opt{rtl} ne 'default';
	$xopt{language} = $opt{lang} if $opt{lang} ne 'default';
	$xopt{pitch} = $font{pitch} if exists $font{pitch};
	$text = $i->text_shape($text, %xopt);
	$text->_debug if ref($text);
}
my @box = @{$i->get_text_box($text)};
my ($xmin,$ymin,$xmax,$ymax) = (0,0,1,1);
for ( my $i = 0; $i < 8; $i += 2 ) {
	my ( $x, $y ) = @box[$i,$i+1];
	$xmin = $x if $xmin > $x;
	$ymin = $y if $ymin > $y;
	$xmax = $x if $xmax < $x;
	$ymax = $y if $ymax < $y;
}

$xmax -= $xmin;
$ymax -= $ymin;
for ( my $i = 0; $i < 8; $i += 2 ) {
	$box[$i]   -= $xmin;
	$box[$i+1] -= $ymin;
}
$i->size($xmax+1, $ymax+1);
$i->backColor(0xffffff) if $opt{box};
$i->clear;
$i->backColor(0x0) if $opt{box};
$i->text_out($text, -$xmin, -$ymin);
$i->type(im::Byte);
my ($w,$h) = $i->size;

my @ascii;
if ( $opt{ascii}) {
	@ascii = (' ', '^', 'v', 'X');
} elsif ( ($ENV{LANG} // '') =~ /utf\-?8/i) {
	@ascii = (' ', chr(0x2580), chr(0x2584), chr(0x2588) );
	binmode STDOUT, ":utf8";
} else {
	use bytes;
	@ascii = (' ', chr(223), chr(220), chr(219) );
	no bytes;
}

for my $j ( map { $_ * 2 } reverse 0 .. ($h/2-1)) {
	my $u = substr( $i->scanline($j),   0, $w );
	my $l = substr( $i->scanline($j+1), 0, $w );
	$u =~ tr/\x{0}\x{ff}/\x{1}\x{0}/;
	$l =~ tr/\x{0}\x{ff}/\x{1}\x{0}/;
	my $p = '';
	for my $k ( 0..$w-1 ) {
		my $x = ord substr($u, $k, 1);
		my $y = ord substr($l, $k, 1);
		$p .= $ascii[$x * 2 + $y];
	}
	print "$p\n";
}
