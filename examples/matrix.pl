use strict;
use Prima::Classes;
use Prima::Label;
use Prima::MsgBox;

$::application = Prima::Application->  create;

my $smp = "The Matrix has you";
my $maxstep = 40;
my $ymaxstep = 60;
my $widefactor = 0.05;
my $shades = 3;
my $shadesDepth = 4;
my $xshspeed = 2;
my $basicfsize = 10;
my $vlines = 40;

my $maxln = length( $smp);
my @vlinst = map { int( rand( $ymaxstep))} 1..$vlines;
my @vlsped = (( 1) x $vlines);
my @vlxcol = (( 0) x $vlines);
my $xshcnt = -1000;
my $xshdir  = 1;
my $xcol = 30;
my $yextraspeed = 0;

my %fsh = ();
my %fhh = ();

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

   if ( $xshcnt > 1000) {
      $xshdir = -1;
   } elsif ( $xshcnt < -1000) {
      $xshdir = 50;
   }
   $xshcnt += $xshdir * $xshspeed;
   $c-> font-> set(
      height    => $oheight,
      width     => $owidth,
      direction => int(($xshcnt * 0.1 + $id / $maxstep * 60) / 10) * 10
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

sub resetfs
{
   my $self = $_[0];
   my @sz = $self-> size;
   my $min = $sz[0] < $sz[1] ? $sz[0] : $sz[1];
   $basicfsize = int( $min / 100);
   $self-> font-> size( $basicfsize);
   $ymaxstep = $sz[1] / $self-> font-> height + length( $smp) * 2;
   @vlxcol = map { int(rand( $sz[0] - 30)) + 15 } 1..$vlines;
}

my $w = Prima::Window-> create(
   palette => [@spal],
   font => { name => 'Courier New', size => $basicfsize, },
   backColor => 0x002000,
   windowState => ws::Maximized,
   color     => cl::LightGreen,
   onDestroy => sub { $::application-> close; },
   onPaint   => sub {
      my ( $self, $c) = @_;
      my @sz = $c-> size;
      my $cc = $self-> color;
      $c-> color( $self-> backColor);
      $c-> bar( 0,0,@sz);
      $self-> {xcnt} = 1 if ++$self-> {xcnt} >= $maxstep;

      for ( 0..$shades) {
         my $x = $self->{xcnt} - (( $shades - $_) * $shadesDepth);
         $x += $maxstep if $x <= 0;
         efont( $c, $x);


         #$x -= ( $shades - $_);
         #$x += $shades;
         #next if $x <= 0;
         $x = $x - (( $shades - $_) * $shadesDepth);
         ecolor( $c, $cc, $self-> backColor, $x / 30);
         $c-> text_out( $smp,
            ( $sz[0] - $c-> get_text_width( $smp)) / 2,
            ( $sz[1] - $c-> font-> height) / 2);
      }

      $c-> font-> set (
         size =>  $basicfsize * 1.5,
         style => fs::Bold,
         direction => 0,
      );
      $c-> color( $cc);

      my $ymans;
      for ( $ymans = 0; $ymans < $vlines; $ymans++) {
         if ( ++$vlinst[ $ymans] >= $ymaxstep) {
            $vlinst[ $ymans] = 1;
            $vlxcol[ $ymans] = int( rand( $sz[0] - 30)) + 15;
            $vlsped[ $ymans] = int(rand( 4)) - 1;
            $vlsped[ $ymans] = 0 if $vlsped[ $ymans] < 0;
            $vlsped[ $ymans] *= 3;
         }
         my $fh = $c-> font-> height;
         my $y = $sz[1] - ($vlinst[ $ymans] - $maxln) * $fh;

         my $i;
         $vlinst[ $ymans] += $vlsped[ $ymans];
         ecolor( $c, $cc, cl::Yellow, 0.5) if $vlsped[ $ymans] > 1;
         for ( $i = 0; $i < $maxln; $i++) {
            $c-> text_out( substr( $smp, $i, 1), $vlxcol[ $ymans], $y);
            $y -= $fh;
         }
         $c-> color( $cc) if $vlsped[ $ymans] > 1;
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