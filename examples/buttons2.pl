use Prima;
use Prima::Const;
use Prima::Widgets;
use Prima::Buttons;
use Prima::StdBitmap;
use Prima::StdDlg;

$::application = Prima::Application-> create( name => "Generic.pm");

my $w = Prima::Window-> create(
  text=> "Handmade buttons",
  size => [ 300, 200],
  onDestroy => sub { $::application-> close},
  centered => 1,
  popupItems => [['Hallo!' => '']],
);

$w-> insert( CheckBox =>
   rect => [ 10, 150, 180, 180],
   selectable => 1,
   text => "~Check box",
   hint => 'Check box!',
   popupItems => [['Hallo2!' => '']],
);

$w-> insert( Radio =>
   rect => [ 190, 150, 300, 180],
   selectable => 1,
   text => "~Radio button",
   hint => 'Radio!',
);

$i = Prima::StdBitmap::icon( sbmp::GlyphCancel);
$j = Prima::StdBitmap::icon( sbmp::GlyphOK);

$w-> insert( Button =>
   origin => [ 10, 100],
   text => "~Default style",
   default => 1,
   hint => 'Nice?',
);

$w-> insert( Button =>
   origin  => [ 10, 55],
   text => "~BitBlt",
   image   => $i,
   glyphs  => 2,
   hint => 'Sly button',
);

$w-> insert( Button =>
   origin  => [ 10, 10],
   text => "Disab~led",
   image   => $i,
   glyphs  => 2,
   enabled => 0,
   hint => 'See me?',
);


$w-> insert( Button =>
   origin     => [ 130, 10],
   text    => "Bits for toolbar",
   image      => $i,
   glyphs     => 2,
   vertical   => 1,
   height     => 120,
   imageScale => 3,
   selectable => 0,
   flat       => 1,
   hint       => "Pressing this button copies it\'s image\n into clipboard",
   onClick    => sub {
      my $self = $_[0];
      unless ( $self-> get_shift_state & km::Ctrl) {
         my $i = Prima::Image-> create(
            width  => $self-> width,
            height => $self-> height,
            font   => $self-> font,
            type   => im::Byte,
         );
         $i-> begin_paint;
         $self-> notify(q(Paint), $i);
         $i-> end_paint;
         $::application-> clipboard-> image($i);
      } else {
         my $i = $::application-> clipboard-> image;
         $self-> set(
            glyphs     => 1,
            imageScale => 1,
            image      => $i,
         ) if $i;
      }
   },
);

$w-> insert( SpeedButton =>
   origin     => [ 250, 10],
   image      => $j,
   glyphs     => 2,
   text    => 0,
   checkable  => 1,
   holdGlyph  => 1,
   hint       => 'C\'mon, press me!',
);


run Prima;
