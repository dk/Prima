use strict;
use Prima qw(ScrollBar);

package MyWindow;
use vars qw(@ISA);
@ISA = qw(Prima::Window);

sub updateArea
{
  $_[0]-> Area-> backColor(
     $_[0]-> Blue-> value|($_[0]-> Green-> value<<8)|($_[0]-> Red-> value<<16)
  );
}

sub Timer1_Tick
{
   $_[ 0]-> Red  -> backColor(( $_[ 0]-> Red  -> backColor == cl::Red  ) ? cl::LightRed   : cl::Red  );
   $_[ 0]-> Green-> backColor(( $_[ 0]-> Green-> backColor == cl::Green) ? cl::LightGreen : cl::Green);
   $_[ 0]-> Blue -> backColor(( $_[ 0]-> Blue -> backColor == cl::Blue ) ? cl::LightBlue  : cl::Blue );
}

package UserInit;

$::application = Prima::Application-> create( name => "scrollbars.pm");
my $w = MyWindow-> create(
   text => "Scrollbar & timer example",
   left    => 100,
   bottom  => 100,
   width   => 300,
   height  => 300,
   borderStyle => bs::Sizeable,
   onDestroy => sub {$::application-> close},
);

$w-> insert(
   "ScrollBar",
    name     => "Red",
    origin   => [ 2, 20],
    vertical => 0,
    max      => 255,
    width    => $w-> width - 2,
    onChange => sub { $w-> updateArea },
);
$w-> insert(
   "ScrollBar",
    name     => "Green",
    origin   => [ 2, 40],
    vertical => 0,
    max      => 255,
    width    => $w-> width - 2,
    onChange => sub { $w-> updateArea },
);
$w-> insert(
   "ScrollBar",
    name     => "Blue",
    origin   => [ 2, 60],
    vertical => 0,
    max      => 255,
    width    => $w-> width - 2,
    onChange => sub { $w-> updateArea },
);

$w-> insert(
   "ScrollBar",
    name     => "Bluex",
    origin   => [ 2, 80],
    vertical => 1,
    max      => 255,
    height    => $w-> height - 80,
);


$w-> insert(
   "Widget",
   name      => "Area",
   rect      => [ 20, 100, 280, 280],
   backColor => cl::Black,
   growMode  => gm::GrowHiX | gm::GrowHiY,
   sizeMin   => [120, 120],
   sizeMax   => [420, 420],
   onPaint => sub
   {
      my ($x,$y)=$_[0]->size;
      $_[0]->color($_[0]->backColor);
      $_[0]-> bar(0,0,$x,$y);
      $_[0]->color(cl::White);
      $_[0]->rop(rop::XorPut);
      $_[0]->line(0,0,$x,$y);
      $_[0]->line(0,$y,$x,0);
   },
   onMouseDown=>sub{
      $_[0]->height($_[0]->height+(($_[1]==1)?1:-1));
   },
);
my $t = $w-> insert(
   Timer=>
   timeout=> 2000,
   name => 'Timer1',
   delegations => ['Tick'],
);
$t-> start;

run Prima;
