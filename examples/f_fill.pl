use strict;
use Prima;
use Prima::Classes;
use Prima::Label;
use Prima::MsgBox;

$::application = Prima::Application->  create;

my $i = Prima::Image-> create(
  preserveType => 1,
  type => im::BW,
  font => { size => 100, style => fs::Bold|fs::Italic },
);
$i-> begin_paint_info;
my $textx = $i-> get_text_width( "PRIMA");
my $texty = $i-> font-> height;
$i-> end_paint_info;
$i-> size( $textx + 20, $texty + 20);

my @is = $i-> size;
$i-> begin_paint;
$i-> color( cl::Black);
$i-> bar(0,0,@is);
$i-> color( cl::White);
$i-> text_out( "PRIMA", 0,0);
$i-> end_paint;


my @xpal = ();
for ( 1..32) {
   my $x = (32-$_) * 8;
   push(@xpal, $x,$x,$x);
};

my $w = Prima::Window-> create(
   onDestroy=> sub {$::application-> close;},
   size   => [ @is],
   centered => 1,
   buffered => 1,
   palette => [ @xpal],
   onPaint => sub {
     my ( $self, $canvas) = @_;
     $canvas-> color( cl::Back);
     $canvas-> bar( 0, 0, $canvas-> size);
     my $xrad = $is[0] / 62;
     for ( 1..32) {
        my $x = (32-$_) * 8;
        $x = ($x<<16)|($x<<8)|$x;
        $canvas-> color($x);
        $canvas-> fill_ellipse($is[0]/2,$is[1]/2, $xrad*(32-$_), $xrad*(32-$_));
     };
     $canvas-> region( $i);
     for ( 1..32) {
        my $x = ($_-1) * 8;
        $x = ($x<<16)|($x<<8)|$x;
        $canvas-> color($x);
        $canvas-> fill_ellipse($is[0]/2,$is[1]/2, $xrad*(32-$_), $xrad*(32-$_));
     };
  },
);

run Prima;