
use strict;
use Carp;
use Prima;
use Prima::Classes;
use Prima::Buttons;

package UserButton;
use vars qw(@ISA);
@ISA = qw(Prima::Button);

sub on_click
{
   print "User button has been pressed";
}

sub on_paint
{
  my ($self,$canvas) = @_;
  my ($x, $y) = $canvas-> get_size;
  my @fcap = ( $canvas-> get_text_width( $self-> text), $canvas-> font->height);
  $canvas-> color( $self-> pressed ? cl::LightGreen : cl::Green);
  $canvas-> bar(0, 0, $x - 1 , $y - 1);

  $canvas-> color( $self-> pressed ? cl::Green : cl::LightGreen);
  $canvas-> line ( 0, 0, 0, $y - 1);
  $canvas-> line ( 1, 1, 1, $y - 2);
  $canvas-> line ( 0, $y - 1, $x - 1, $y - 1);
  $canvas-> line ( 1, $y - 2, $x - 2, $y - 2);
  $canvas-> color( $self-> pressed ? cl::White : cl::Gray);
  $canvas-> line ( 0, 0, $x - 1, 0);
  $canvas-> line ( 1, 1, $x - 2, 1);
  $canvas-> line ( $x - 1, 0, $x - 1, $y - 1);
  $canvas-> line ( $x - 2, 1, $x - 2, $y - 2);

  $canvas-> color(cl::Black);
  $canvas-> text_out ( $self-> text,
    ($x - $fcap[ 0]) / 2,
    ($y - $fcap[ 1]) / 2
  );
  if ( $self-> default) {
    $canvas-> linePattern( lp::Dot);
    $canvas-> rectangle(
      ($x - $fcap[ 0]) / 2 - 3,
      ($y - $fcap[ 1]) / 2 - 3,
      ($x + $fcap[ 0]) / 2 + 3,
      ($y + $fcap[ 1]) / 2 + 3
    );
    $canvas-> linePattern( lp::Solid);
  }
}


package UserInit;

$::application = Prima::Application-> create( name => "Buttons.pm");
my $w = Prima::Window-> create(
   onDestroy => sub { $::application-> close} ,
   text   => "Button example",
   centered  => 1,
   height    => 300,
   width     => 400,
);
$w-> insert( "Button"     , origin => [  50,180], pressed => 1);
$w-> insert( "UserButton" , origin => [ 250,180]);
$w-> insert( "Radio"      , origin => [  50,140]);

run Prima;
