use strict;
use Prima;
use Prima::Buttons;
use Prima::Classes;
use Prima::StdBitmap;

package UserInit;
$::application = Prima::Application-> create( name => "Generic.pm");

my $w = Prima::Window-> create(
   size    => [ 190, 460],
   left    => 200,
   onDestroy => sub {$::application-> close},
   text    => 'Pointers',
);

my %grep_out = (
   'BEGIN' => 1,
   'END' => 1,
   'AUTOLOAD' => 1,
   'constant' => 1
);

my @a = sort { &{$cr::{$a}}() <=> &{$cr::{$b}}()} grep { !exists $grep_out{$_}} keys %cr::;
shift @a;
$w-> pointerVisible(0);
my $i;

for ( $i = cr::Arrow; $i <= cr::Invalid; $i++)
{
   $w-> pointer( $i);
   my $b = $w-> insert( Button =>
      flat => 1,
      left    => 10,
      width   => 160,
      bottom  => $w-> height - ($i+1) * 40 - 10,
      pointer => $i,
      name    => shift @a,
      image   => $w-> pointerIcon,
      onClick => sub { $::application-> pointer( $_[0]-> pointer); },
   );
};

my $ptr = Prima::StdBitmap::icon( sbmp::DriveCDROM);

my $b = $w-> insert( SpeedButton =>
   left    => 10,
   width   => 160,
   bottom  => $w-> height - (cr::User+1) * 40 - 10,
   pointer => $ptr,
   image   => $ptr,
   text => $a[-1],
   flat => 1,
   onClick => sub {
       $::application-> pointer( $_[0]-> pointer);
   },
);

$w-> pointer( cr::Default);
$w-> pointerVisible(1);

run Prima;
