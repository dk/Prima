
use Prima::Classes;
use Prima::Buttons;
use Prima::ScrollBar;

package UserInit;

$::application = Prima::Application->create(name=> "rtc");
my $w = Prima::Window-> create(
  text=> "Test of RTC",
  origin => [ 200, 200],
  size   => [ 250, 300],
  onDestroy => sub {$::application-> close},
);

$w-> insert( "Button",
  origin  => [10, 10],
  width   => 220,
  text => "Change scrollbar direction",
  onClick=> sub {
    my $i = $_[0]-> owner-> govno;
    $i-> vertical( ! $i-> vertical);
  }
);

$w-> insert( "ScrollBar",
  name    => "govno",
  origin  => [ 40, 80],
  size    => [150, 150],
  onCreate => sub {
     Prima::Timer-> create(
         timeout=> 1000,
         timeout=> 200,
         owner  => $_[0],
         onTick => sub{
            # $_[0]-> owner-> vertical( !$_[0]-> owner-> vertical);
            my $t = $_[0]-> owner;
            my $v = $t-> partial;
            $t->partial( $v+1);
            $t->partial(1) if $t-> partial == $v;
            #$_[0]-> timeout( $_[0]-> timeout == 1000 ? 200 : 1000);
         },

     )-> start;
  },
);

run Prima;
