use Prima;
use Prima::Application;
use Prima::Buttons;
use Prima::Label;

my $w = Prima::Window-> create(
   centered => 1,
   size     => [ 300, 200],
   text     => 'Survey',
   sizeMin  => [ 150, 100],
);

my $stage = 0;

$w-> insert( [ Button => 
   text   => 'Yes',
   origin => [ 20, 20],
   onClick => sub {
      if ( $stage) {
         $::application-> close;
      } else {   
         $w-> No-> destroy;
         $w-> Label-> text("We never doubt that!");
         $_[0]-> x_centered(1);
         $_[0]-> text("OK");
         $stage = 1;
      }
   },   
], [ Button => 
   text   => 'No',
   origin => [ 180, 20],
   growMode => gm::GrowLoX,
   tabStop => 0,
   selectable => 0,
   name     => 'No',
   onMouseMove => sub {
       my ( $self, $mod, $x, $y) = @_;
       my @ms = $self-> owner-> size;
       my @sz = $self-> size;
       my @o  = $self-> origin;
       my @ra = (( $x > $sz[0]/2) ? -1 : 1, ( $y > $sz[0]/2) ? -1 : 1);
       ( $x, $y) = ( $x + $o[0], $y + $o[1]);
       for(0,1){$o[$_] = $ms[$_] - $sz[$_] if $o[$_] + $sz[$_] >= $ms[$_]};
       while ( 1) {
AGAIN:          
          for ( 0, 1) {
             $o[$_] += $ra[$_]; 
             $ra[$_] = (( $ra[$_] > 0) ? -1 : 1), goto AGAIN if $o[$_] + $sz[$_] >= $ms[$_] || $o[$_] < 0;
          }
          last if ( $x < $o[0] || $x >= $o[0] + $sz[0]) && 
                  ( $y < $o[1] || $y >= $o[1] + $sz[1]);
       }   
       $self-> origin( @o);
   },
   onMouseDown => sub { $_[0]-> clear_event, },
], [ Label => 
   name     => 'Label',
   origin   => [ 20, 80],
   size     => [ 260, 100],
   text     => "Are you satisfied with your salary?",
   alignment => ta::Center,
   valignment => ta::Center,
   growMode => gm::Client,
   wordWrap => 1,
]);
$w-> No-> bring_to_front;
run Prima;
