use Prima;
use Prima::Const;
use Prima::Application name => 'Generic.pm';

$w = Prima::Window-> create(
   size => [ 300, 300],
   borderStyle => bs::Dialog,
   backColor => cl::Green,
   onCreate => sub {
      $_[0]-> {delta} = 0;
   },
   onPaint => sub {
      my ($self,$canvas) = @_;
      my $c = $self-> color;
      $canvas-> color( $self-> backColor);
      $canvas-> bar(0,0,$self-> size);
      $canvas-> color( $c);
      my $d = $self-> {delta};
      my $i;
      for ( $i = -1; $i < 7; $i++)
      {
         $canvas-> text_out("Hello!", $d + $i * 40, $d + $i * 40);
      }
   },
);

$w-> insert( Timer =>
   timeout => 100,
   onTick  => sub {
      $w-> {delta} += 2;
      $w->{ delta} = 0 if $w->{ delta} >= 40;
      $w-> repaint;
   }
)-> start;

$w-> insert(
   Widget =>
   left => 100,
   transparent => 1,
   onPaint => sub {
      my $self = $_[0];
      $self-> color( cl::LightGreen);
      $self-> lineWidth( 4);
      $self-> line( 3, 3, $self-> size);
      $self-> ellipse( 50, 50, 40, 40);
   },
   onMouseDown => sub {
      $_[0]-> bring_to_front;
   },
);

my $tt = Prima::Widget-> create(
   name => 'W1',
   onPaint => sub {
      $_[1]-> color( cl::LightRed);
      $_[1]-> font-> size( 36);
      $_[1]-> text_out("Hello!", 0, 0);
   },
   onMouseDown => sub {
     $_[0]->{drag}    = [ $_[3], $_[4]];
     $_[0]->{lastPos} = [ $_[0]-> left, $_[0]-> bottom];
     $_[0]-> capture(1);
     $_[0]-> repaint;
   },
   onMouseMove => sub{
     if ( exists $_[0]->{ drag})
     {
        my @org = $_[0]-> origin;
        my @st  = @{$_[0]->{drag}};
        my @new = ( $org[0] + $_[2] - $st[0], $org[1] + $_[3] - $st[1]);
        $_[0]-> origin( $new[0], $new[1]) if $org[1] != $new[1] || $org[0] != $new[0];
     }
   },
   onMouseUp => sub {
     $_[0]-> capture(0);
     delete $_[0]->{drag};
   },
);

my $i = Prima::Image-> create( width => $tt-> width, height => $tt-> height,
type => im::BW, conversion => ict::None);
$i-> begin_paint;
$i-> color( cl::Black);
$i-> bar(0,0,$i-> size);
$i-> color( cl::White);
$i-> font-> size( 36);
$i-> text_out("Hello!", 0, 0);
$i-> end_paint;
$tt-> shape($i);
$tt-> bring_to_front;

run Prima;
