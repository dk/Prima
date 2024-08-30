=pod

=head1 NAME

examples/matrix.pl - matrix screen-saver

=head1 FEATURES

Tests implementation of the paletted DeviceBitmap
and performance of large fonts

=cut

use strict;
use warnings;
use Prima;
use Prima::Application;

my $smp = "The Matrix has you";
my $maxstep = 40;
my $ymaxstep = 60;
my $widefactor = 0.05;   # range 0.01 - 0.3
my $digitShades = 8;     # range 1 - 20
my $textShades = 3;      # range 1 - 20
my $shadesDepth = 4;     # range 1 - 100
my $xshspeed = 0.2;      # range 1 - 4
my $basicfsize = 10;     # range 6 - 24
my $vlines = 40;         # range 10 - 80
my $textToBMRatio = 0.3; # range 0.01 - 0.9
my $digitTicks    = 150; # range 1-...
my $textTicks     = 30;  # range 1-...


my $maxln = length( $smp);
my @vlinst = map { int( rand( $ymaxstep))} 1..$vlines;
my @vlbminst = map { int( rand( $ymaxstep))} 1..$vlines;
my @vlsped = (( 1) x $vlines);
my @vlbmsped = (( 1) x $vlines);
my @vlbms  = map { int( rand( 3))} 1..$vlines;
my @vlxcol = (( 0) x $vlines);
my @vlbmxcol = (( 0) x $vlines);
my $xshcnt = -100;
my $xshdir  = 1;
my $xcol = 30;
my $yextraspeed = 0;
my $ticker = 10000000;
my $tickerMode = 0;
my $shades = 0;
my $showBigText = 1;
my $showSmallText = 1;
my $showBitmaps   = 1;
my $fullScreen = 0;


my %fsh  = ();
my %fhh  = ();
my @dbms = ();

sub efont
{
	my ( $c, $id) = @_;
	my $oheight;
	if ( exists $fsh{ $basicfsize}) {
		$oheight = $fsh{ $basicfsize};
	} else {
		$c-> font-> size( $basicfsize);
		$oheight = $c-> font-> height;
		$fsh{ $basicfsize} = $oheight;
	}

	$oheight = int( $oheight * ( 2 ** ( $id / 6)));
	my $owidth;
	if ( exists $fhh{ $oheight}) {
		$owidth = $fhh{ $oheight};
	} else {
		$c-> font-> height( $oheight);
		$owidth = $c-> font-> width;
		$fhh{$oheight} = $owidth;
	}

	$owidth = $owidth * $id * $widefactor;
	$owidth = ( $owidth < 1) ? 1 : $owidth;

	if ( $xshcnt > 100) {
		$xshdir = -0.1;
	} elsif ( $xshcnt < -100) {
		$xshdir = 5;
	}
	$xshcnt += $xshdir * $xshspeed;
	$c-> font-> set(
		height    => $oheight,
		width     => $owidth,
		direction => ($xshcnt + $id / $maxstep * 6) / 10,
	);
}

sub ecolor
{
	my ( $c, $f, $b, $p) = @_;
	$p = 1 if $p > 1;
	$p = 0 if $p < 0;
	$p =
	(((( $f >> 16) * $p) + (( $b >> 16) * ( 1 - $p))) << 16) |
	((((( $f >> 8) & 0xFF) * $p) + ((( $b >> 8) & 0xFF) * ( 1 - $p))) << 8)|
	((( $f & 0xFF) * $p) + (( $b & 0xFF) * ( 1 - $p)));
	$c-> color( $p);
}

my $i;
my @spal = ();
for ( $i = 0; $i < 256; $i++) {
	push( @spal, 0, $i, 0);
};
my @gifs = map { Prima::Image-> load( 'matrix.gif', index => $_)} 0..2;
@gifs = () unless $gifs[0];
my @wsaverect;


sub resetfs
{
	my $self = $_[0];
	my @sz = $self-> size;
	my $min = $sz[0] < $sz[1] ? $sz[0] : $sz[1];
	$basicfsize = int( $min / 100);
	$self-> font-> size( $basicfsize);
	$ymaxstep = $sz[1] / $self-> font-> height + length( $smp) * 2;
	@vlxcol = map { int(rand( $sz[0] - 30)) + 15 } 1..$vlines;
	@vlbmxcol = map { int(rand( $sz[0] - 30)) + 15 } 1..$vlines;
	my $fw = $self-> font-> height;

	@dbms = map {
		my $x = $_-> dup;
		$x-> size( $fw, $_-> height * $fw / 21);
		$x-> bitmap;
	} @gifs;
}


my $w = Prima::MainWindow-> new(
	palette => [@spal],
	font => { name => 'Courier New', size => $basicfsize, },
	backColor => 0x002000,
	windowState => ws::Maximized,
	color     => cl::LightGreen,
	menuItems => [
		["~Options" => [
		[ '@*bt' => 'Show ~big text' => sub { $showBigText = $_[2]; }],
		[ '@*st' => 'Show ~small text' => sub { $showSmallText = $_[2]; }],
		[ '@*bm' => 'Show bit~maps' => sub { $showBitmaps = $_[2] }],
		[],
		['~Full screen' => sub {
			$fullScreen = 1;
			@wsaverect = $_[0]-> rect;
			$_[0]-> rect( 0, 0, $_[0]-> owner-> size);
			}, ],
		]],
	],
	onKeyDown => sub {
		return unless $fullScreen;
		$fullScreen = 0;
		$_[0]-> rect( @wsaverect);
	},
	onMouseDown => sub {
		return unless $fullScreen;
		$fullScreen = 0;
		$_[0]-> rect( @wsaverect);
	},
	onPaint   => sub {
		my ( $self, $c) = @_;
		my @sz = $c-> size;
		my $cc = $self-> color;

		$ticker++;
		my $lim = $tickerMode ? $digitTicks : $textTicks;
		if ( $ticker > $lim) {
			$ticker = 0;
			$tickerMode = !$tickerMode;
			$shades = $tickerMode ? $digitShades : $textShades;
		}

		if ( $tickerMode || ( $ticker % 2) || ( $ticker < $textTicks / 2)) {
			$c-> color( $self-> backColor);
		} else {
			$c-> color( 0x00F000);
		}
		$c-> bar( 0,0,@sz);
		$self-> {xcnt} = 1 if ++$self-> {xcnt} >= $maxstep;

		my $ymans;
		my $fh = $c-> font-> height;
		if ( $showBitmaps) {
			for ( $ymans = 0; $ymans < $vlines; $ymans++) {
				my $y = $sz[1] - $vlbminst[ $ymans] * $fh;
				$c-> put_image( $sz[0] - $vlbmxcol[ $ymans], $y,
					$dbms[ $vlbms[ $ymans]]);
				if ( ++$vlbminst[ $ymans] >= $ymaxstep) {
					$vlbminst[ $ymans] = 1;
					$vlbmxcol[ $ymans] = int( rand( $sz[0] - 30)) + 15;
					$vlbmsped[ $ymans] = rand( 3) - 1;
					$vlbmsped[ $ymans] = 0 if $vlbmsped[ $ymans] < 0;
					$vlbmsped[ $ymans] *= 3;

				}
				$vlbminst[ $ymans] += $vlbmsped[ $ymans];
			}
		}

		if ( $showBigText) {
			for ( 0..$shades) {
				my $x = $self-> {xcnt} - (( $shades - $_) * $shadesDepth);
				$x += $maxstep if $x <= 0;
				efont( $c, $x);

				#$x -= ( $shades - $_);
				#$x += $shades;
				#next if $x <= 0;
				$x = $x - (( $shades - $_) * $shadesDepth);
				ecolor( $c, $cc, $self-> backColor, $x / 30);
				my $mp;
				if ( $tickerMode) {
					$mp = abs( 10 * int( $c-> font-> direction));
					if ( $mp < 100) {
						$mp = $mp * 10 + $mp / 10;
					} else {
						$mp = $mp * 100 + reverse($mp / 10);
					}
				} else {
					$mp = $smp;
				}

				$c-> text_out( $mp,
					( $sz[0] - $c-> get_text_width( $mp)) / 2,
					( $sz[1] - $c-> font-> height) / 2);
			}
		}

		$c-> font-> set (
			size =>  $basicfsize * 1.5,
			style => fs::Bold,
			direction => 0,
		);
		$c-> color( $cc);

		$fh = $c-> font-> height;
		if ( $showSmallText) {
			for ( $ymans = 0; $ymans < $vlines * $textToBMRatio; $ymans++) {
				if ( ++$vlinst[ $ymans] >= $ymaxstep) {
					$vlinst[ $ymans] = 1;
					$vlxcol[ $ymans] = int( rand( $sz[0] - 30)) + 15;
					$vlsped[ $ymans] = rand( 3) - 1;
					$vlsped[ $ymans] = 0 if $vlsped[ $ymans] < 0;
					$vlsped[ $ymans] *= 3;
				}
				my $y = $sz[1] - ($vlinst[ $ymans] - $maxln) * $fh;

				my $i;
				$vlinst[ $ymans] += $vlsped[ $ymans];
				ecolor( $c, $cc, cl::Yellow, 0.5) if $vlsped[ $ymans] > 1;
				$y = $sz[1] - ($vlinst[ $ymans] - $maxln) * $fh;
				for ( $i = 0; $i < $maxln; $i++) {
					$c-> text_out( substr( $smp, $i, 1),
						$vlxcol[ $ymans] / $textToBMRatio, $y);
					$y -= $fh;
				}
				$c-> color( $cc) if $vlsped[ $ymans] > 1;
			}
		}
	},
	onSize => sub {
		resetfs( $_[0]);
	},
	onCreate => sub {
		resetfs( $_[0]);
	},
	buffered => 1,
);


$w-> insert( Timer =>
	timeout => 50 => onTick => sub {
	$w-> repaint;
})-> start;


run Prima;
