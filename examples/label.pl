use Prima;
use Prima::Const;
use Prima::Buttons;
use Prima::Label;

$::application = Prima::Application-> create( name => "Generic.pm");

my $w = Prima::Window-> create(
  size => [ 430, 200],
  text => "Static texts",
  onDestroy => sub { $::application-> destroy},
);

my $b1 = $w-> insert( Button => left => 20 => bottom => 0);

$w-> insert( Label =>
   origin => [ 20, 50],
   text => "#define inherited CComponent->\n#define my  ((( P~AbstractMenu) self)-> self)->\n #define var (( PAbstractMenu) self)->",
   focusLink => $b1,
   wordWrap => 1,
   height => 80,
   alignment => ta::Center,
   growMode => gm::Client,
   showPartial => 0,
);

my $b2 = $w-> insert( Button =>
  left => 320,
  bottom => 0,
  growMode => gm::GrowLoX,
);

$w-> insert( Label =>
   origin    => [ 320, 50],
   text   => 'Disab~led',
   focusLink => $b2,
   autoHeight => 1,
   enabled   => 0,
   growMode  => gm::GrowLoX,
);


run Prima;
