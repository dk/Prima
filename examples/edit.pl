

use Prima;
use Prima::Const;
use Prima::Widgets;
use Prima::InputLine;

$::application = Prima::Application-> create( name => "Generic.pm");

my $l;
my $w = Prima::Window-> create(
  size => [ 700, 300],
  onDestroy => sub { $::application-> destroy},
  accelItems => [
    [a=>a=> kb::F3=>sub{$l-> repaint}],
  ]

);

$l = $w-> insert( InputLine =>
   text  => '0:::1234 5678 90ab cdef ghij klmn oprq stuv:1::1234 5678 90ab cdef ghij klmn oprq stuv:2::1234 5678 90ab cdef ghij klmn oprq stuv:3::1234 5678 90ab cdef ghij klmn oprq stuv:4::1234 5678 90ab cdef ghij klmn oprq stuv::End',
   y_centered  => 1,
   left   => 200,
   width => 300,
   #selection => [1,12],
   height => 40,
#  maxLen => 15,
   firstChar => 2,
#  offset => 3,
   alignment => ta::Center,
   font   => { size => 18, name => 'Tms Rmn'},
   growMode => gm::GrowHiX,
   buffered => 1,
   onFontChanged => sub {
      $_[0]-> height( $_[0]-> font-> height + 4);
   },
   xonPaint => sub {},
   borderWidth => 3,
   autoSelect => 0,
);

run Prima;
